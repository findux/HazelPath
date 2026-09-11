#include "SimulationController.h"

#include "MotionInterpolator.h"

#include <algorithm>

namespace nc {

void SimulationController::setToolpath(const Toolpath* value) {
	toolpath = value;
	rebuildCommandDurations();
	stop();
}

void SimulationController::rebuildCommandDurations() {
	commandDurations.clear();
	commandEndSeconds.clear();
	if (toolpath == nullptr) return;
	commandDurations.reserve(toolpath->size());
	commandEndSeconds.reserve(toolpath->size());
	double elapsedSeconds = 0.0;
	for (std::size_t index = 0; index < toolpath->size(); ++index) {
		commandDurations.push_back(MotionInterpolator::durationSeconds(
			toolpath->getCommandRef(index), rapidFeedRate, defaultFeedRate));
		elapsedSeconds += commandDurations.back();
		commandEndSeconds.push_back(elapsedSeconds);
	}
}

double SimulationController::commandDuration(std::size_t index) const {
	return index < commandDurations.size() ? commandDurations[index] : 0.0;
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

		const ToolpathCommand& command = toolpath->getCommandRef(commandIndex);
		const double duration = commandDuration(commandIndex);

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
		position = toolpath->getCommandRef(toolpath->size() - 1).end;
		status = SimulationStatus::Finished;
		return;
	}

	const double elapsedSeconds = totalDuration * progress;
	const auto boundary = std::lower_bound(
		commandEndSeconds.begin(), commandEndSeconds.end(), elapsedSeconds);
	commandIndex = static_cast<std::size_t>(boundary - commandEndSeconds.begin());
	const double startSeconds = commandIndex == 0 ? 0.0 : commandEndSeconds[commandIndex - 1];
	commandElapsedSeconds = elapsedSeconds - startSeconds;
	const double duration = commandDuration(commandIndex);
	position = MotionInterpolator::sample(toolpath->getCommandRef(commandIndex),
		duration > 0.0 ? commandElapsedSeconds / duration : 1.0);
	status = keepPlaying ? SimulationStatus::Playing : SimulationStatus::Paused;
}

void SimulationController::seekToCommand(
	std::size_t index,
	double normalizedCommandProgress) {
	if (toolpath == nullptr || index >= toolpath->size()) {
		return;
	}

	const ToolpathCommand& command = toolpath->getCommandRef(index);
	const double commandProgress = std::max(
		0.0, std::min(normalizedCommandProgress, 1.0));
	const double duration = commandDuration(index);

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
	rebuildCommandDurations();
}

void SimulationController::setDefaultFeedRate(double value) {
	defaultFeedRate = std::max(value, 0.0);
	rebuildCommandDurations();
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

	const double duration = commandDuration(commandIndex);
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
	return toolpath->getCommandRef(index).sourceLine;
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

	double elapsedSeconds = commandIndex == 0 ? 0.0 : commandEndSeconds[commandIndex - 1];
	elapsedSeconds += commandElapsedSeconds;
	return std::max(0.0, std::min(elapsedSeconds / totalDuration, 1.0));
}
void SimulationController::resetPosition() {
	position = AxisPosition{};
	if (toolpath != nullptr && !toolpath->empty()) {
		position = toolpath->getCommandRef(0).start;
	}
}

double SimulationController::totalDurationSeconds() const {
	return commandEndSeconds.empty() ? 0.0 : commandEndSeconds.back();
}
} // namespace nc
