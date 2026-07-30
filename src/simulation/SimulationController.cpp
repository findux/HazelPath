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
	if (status == SimulationStatus::Finished) {
		stop();
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

	double remainingSeconds = std::max(deltaSeconds, 0.0) * speed;
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

void SimulationController::setSpeed(double value) {
	speed = std::max(value, 0.0);
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
	if (status == SimulationStatus::Finished) {
		return 1.0;
	}
	if (commandIndex >= toolpath->size()) {
		return 1.0;
	}

	const ToolpathCommand command = toolpath->getCommand(commandIndex);
	const double duration = MotionInterpolator::durationSeconds(
		command, rapidFeedRate, defaultFeedRate);
	const double commandProgress = duration > 0.0
		? std::min(commandElapsedSeconds / duration, 1.0)
		: 0.0;
	return (static_cast<double>(commandIndex) + commandProgress) /
		static_cast<double>(toolpath->size());
}

void SimulationController::resetPosition() {
	position = AxisPosition{};
	if (toolpath != nullptr && !toolpath->empty()) {
		position = toolpath->getCommand(0).start;
	}
}

} // namespace nc
