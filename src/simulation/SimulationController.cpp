#include "SimulationController.h"

#include "MotionInterpolator.h"

#include <algorithm>

namespace nc {

void SimulationController::setToolpath(const Toolpath* value) {
	toolpath = value;
	stop();
}

void SimulationController::play() {
	if (toolpath == nullptr || toolpath->empty()) {
		return;
	}
	if (status == SimulationStatus::Finished && speed >= 0.0) {
		stop();
	} else if (status == SimulationStatus::Stopped && speed < 0.0) {
		seek(1.0);
	}
	status = SimulationStatus::Playing;
}
void SimulationController::pause() {
	if (status == SimulationStatus::Playing) {
		status = SimulationStatus::Paused;
	}
}

void SimulationController::stop() {
	status = SimulationStatus::Stopped;
	commandIndex = 0;
	commandElapsedSeconds = 0.0;
	resetPosition();
}

void SimulationController::update(double deltaSeconds) {
	if (status != SimulationStatus::Playing || toolpath == nullptr) {
		return;
	}

	const double frameSeconds = std::max(deltaSeconds, 0.0);
	if (speed < 0.0) {
		const double totalDuration = totalDurationSeconds();
		if (totalDuration <= 0.0) {
			return;
		}
		const double targetProgress = getProgress() + frameSeconds * speed / totalDuration;
		if (targetProgress <= 0.0) {
			seek(0.0);
			status = SimulationStatus::Stopped;
		} else {
			seek(targetProgress);
		}
		return;
	}

	double remainingSeconds = frameSeconds * speed;
	while (status == SimulationStatus::Playing) {
		if (commandIndex >= toolpath->size()) {
			status = SimulationStatus::Finished;
			break;
		}

		const ToolpathCommand command = toolpath->getCommand(commandIndex);
		const double duration = MotionInterpolator::durationSeconds(
			command, rapidFeedRate, defaultFeedRate);

		if (duration <= 0.0) {
			position = command.end;
			++commandIndex;
			commandElapsedSeconds = 0.0;
			continue;
		}

		const double availableSeconds = duration - commandElapsedSeconds;
		const double consumedSeconds = std::min(remainingSeconds, availableSeconds);
		commandElapsedSeconds += consumedSeconds;
		remainingSeconds -= consumedSeconds;
		position = MotionInterpolator::sample(
			command, commandElapsedSeconds / duration);

		if (commandElapsedSeconds >= duration) {
			position = command.end;
			++commandIndex;
			commandElapsedSeconds = 0.0;
			continue;
		}
		break;
	}
}

void SimulationController::seek(double normalizedProgress) {
	if (toolpath == nullptr || toolpath->empty()) {
		return;
	}

	const bool keepPlaying = status == SimulationStatus::Playing;
	const double progress = std::max(0.0, std::min(normalizedProgress, 1.0));
	if (progress <= 0.0) {
		commandIndex = 0;
		commandElapsedSeconds = 0.0;
		resetPosition();
		status = keepPlaying ? SimulationStatus::Playing : SimulationStatus::Stopped;
		return;
	}

	const double totalDuration = totalDurationSeconds();
	if (progress >= 1.0 || totalDuration <= 0.0) {
		commandIndex = toolpath->size();
		commandElapsedSeconds = 0.0;
		position = toolpath->getCommand(toolpath->size() - 1).end;
		status = SimulationStatus::Finished;
		return;
	}

	double remainingSeconds = totalDuration * progress;
	position = toolpath->getCommand(0).start;
	for (std::size_t index = 0; index < toolpath->size(); ++index) {
		const ToolpathCommand command = toolpath->getCommand(index);
		const double duration = MotionInterpolator::durationSeconds(
			command, rapidFeedRate, defaultFeedRate);
		if (duration <= 0.0) {
			position = command.end;
			continue;
		}
		if (remainingSeconds <= duration) {
			commandIndex = index;
			commandElapsedSeconds = remainingSeconds;
			position = MotionInterpolator::sample(command, remainingSeconds / duration);
			status = keepPlaying ? SimulationStatus::Playing : SimulationStatus::Paused;
			return;
		}
		remainingSeconds -= duration;
		position = command.end;
	}
}

void SimulationController::seekToCommand(
	std::size_t index,
	double normalizedCommandProgress) {
	if (toolpath == nullptr || index >= toolpath->size()) {
		return;
	}

	const ToolpathCommand command = toolpath->getCommand(index);
	const double commandProgress = std::max(
		0.0, std::min(normalizedCommandProgress, 1.0));
	const double duration = MotionInterpolator::durationSeconds(
		command, rapidFeedRate, defaultFeedRate);

	commandIndex = index;
	commandElapsedSeconds = duration * commandProgress;
	position = MotionInterpolator::sample(command, commandProgress);
	status = SimulationStatus::Paused;
}

void SimulationController::setSpeed(double value) {
	speed = value;
}

double SimulationController::getSpeed() const { return speed; }

void SimulationController::setRapidFeedRate(double value) {
	rapidFeedRate = std::max(value, 0.0);
}

void SimulationController::setDefaultFeedRate(double value) {
	defaultFeedRate = std::max(value, 0.0);
}

SimulationStatus SimulationController::getStatus() const { return status; }
const AxisPosition& SimulationController::getPosition() const { return position; }
std::size_t SimulationController::getCommandIndex() const { return commandIndex; }

double SimulationController::getCommandProgress() const {
	if (toolpath == nullptr || toolpath->empty()) {
		return 0.0;
	}
	if (status == SimulationStatus::Finished || commandIndex >= toolpath->size()) {
		return 1.0;
	}

	const double duration = MotionInterpolator::durationSeconds(
		toolpath->getCommand(commandIndex), rapidFeedRate, defaultFeedRate);
	if (duration <= 0.0) {
		return 0.0;
	}
	return std::max(0.0, std::min(commandElapsedSeconds / duration, 1.0));
}

int SimulationController::getSourceLine() const {
	if (toolpath == nullptr || toolpath->empty()) {
		return -1;
	}
	const std::size_t index = std::min(commandIndex, toolpath->size() - 1);
	return toolpath->getCommand(index).sourceLine;
}

double SimulationController::getProgress() const {
	if (toolpath == nullptr || toolpath->empty()) {
		return 0.0;
	}
	if (status == SimulationStatus::Finished || commandIndex >= toolpath->size()) {
		return 1.0;
	}

	const double totalDuration = totalDurationSeconds();
	if (totalDuration <= 0.0) {
		return 0.0;
	}

	double elapsedSeconds = 0.0;
	for (std::size_t index = 0; index < commandIndex; ++index) {
		elapsedSeconds += MotionInterpolator::durationSeconds(
			toolpath->getCommand(index), rapidFeedRate, defaultFeedRate);
	}
	elapsedSeconds += commandElapsedSeconds;
	return std::max(0.0, std::min(elapsedSeconds / totalDuration, 1.0));
}
void SimulationController::resetPosition() {
	position = AxisPosition{};
	if (toolpath != nullptr && !toolpath->empty()) {
		position = toolpath->getCommand(0).start;
	}
}

double SimulationController::totalDurationSeconds() const {
	if (toolpath == nullptr) {
		return 0.0;
	}

	double total = 0.0;
	for (std::size_t index = 0; index < toolpath->size(); ++index) {
		total += MotionInterpolator::durationSeconds(
			toolpath->getCommand(index), rapidFeedRate, defaultFeedRate);
	}
	return total;
}
} // namespace nc
