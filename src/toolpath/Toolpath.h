#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace nc {

enum class Units {
	Millimeters,
	Inches
};

enum class MotionType {
	Rapid,
	Linear,
	ArcClockwise,
	ArcCounterClockwise,
	Dwell,
	ToolChange,
	Spindle,
	Custom
};

enum class ArcPlane {
	XY,
	XZ,
	YZ
};

struct AxisPosition {
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	double a = 0.0;
	double b = 0.0;
	double c = 0.0;

	AxisPosition() = default;
	AxisPosition(double xValue, double yValue, double zValue)
		: x(xValue), y(yValue), z(zValue) {}
};

struct ToolpathCommand {
	MotionType type = MotionType::Rapid;
	AxisPosition start;
	AxisPosition end;
	AxisPosition center;
	ArcPlane plane = ArcPlane::XY;
	double feedRate = 0.0;
	double spindleSpeed = 0.0;
	double dwellSeconds = 0.0;
	int toolNumber = 0;
	int sourceLine = -1;
	std::string sourceText;
	std::string customType;
	std::unordered_map<std::string, std::string> metadata;

	void setMetadata(const std::string& key, const std::string& value);
	std::string getMetadata(const std::string& key) const;
};

class Toolpath {
public:
	void clear();
	std::size_t size() const;
	bool empty() const;

	void setCurrentPosition(const AxisPosition& position);
	const AxisPosition& getCurrentPosition() const;

	void addCommand(const ToolpathCommand& command);
	void addRapid(const AxisPosition& end, int sourceLine, const std::string& sourceText);
	void addLinear(const AxisPosition& end, double feedRate, int sourceLine, const std::string& sourceText);
	void addArc(const AxisPosition& end, const AxisPosition& center, bool clockwise,
		ArcPlane plane, double feedRate, int sourceLine, const std::string& sourceText);
	void addDwell(double seconds, int sourceLine, const std::string& sourceText);
	void addToolChange(int toolNumber, int sourceLine, const std::string& sourceText);
	void addSpindle(double speed, int sourceLine, const std::string& sourceText);

	ToolpathCommand getCommand(std::size_t index) const;

	void setMetadata(const std::string& key, const std::string& value);
	std::string getMetadata(const std::string& key) const;

	Units units = Units::Millimeters;
	std::string machineName;
	std::string sourceName;

private:
	AxisPosition currentPosition;
	std::vector<ToolpathCommand> commands;
	std::unordered_map<std::string, std::string> metadata;
};

} // namespace nc