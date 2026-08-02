#pragma once

#include "toolpath/Toolpath.h"

#include <cstddef>

namespace nc {

enum class SimulationStatus {
	Stopped,
	Playing,
	Paused,
	Finished
};

class SimulationController {
public:
	void setToolpath(const Toolpath* value);
	void play();
	void pause();
	void stop();
	void update(double deltaSeconds);
	void seek(double normalizedProgress);
	void seekToCommand(std::size_t index, double normalizedCommandProgress);

	void setSpeed(double value);
	double getSpeed() const;
	void setRapidFeedRate(double value);
	void setDefaultFeedRate(double value);

	SimulationStatus getStatus() const;
	const AxisPosition& getPosition() const;
	std::size_t getCommandIndex() const;
	double getCommandProgress() const;
	int getSourceLine() const;
	double getProgress() const;

private:
	void resetPosition();
	double totalDurationSeconds() const;

	const Toolpath* toolpath = nullptr;
	SimulationStatus status = SimulationStatus::Stopped;
	AxisPosition position;
	std::size_t commandIndex = 0;
	double commandElapsedSeconds = 0.0;
	double speed = 1.0;
	double rapidFeedRate = 3000.0;
	double defaultFeedRate = 1000.0;
};

} // namespace nc
