#include "Toolpath.h"

#include <stdexcept>

namespace nc {

void ToolpathCommand::setMetadata(const std::string& key, const std::string& value) {
	metadata[key] = value;
}

std::string ToolpathCommand::getMetadata(const std::string& key) const {
	const auto found = metadata.find(key);
	return found == metadata.end() ? std::string{} : found->second;
}

void Toolpath::clear() {
	commands.clear();
	currentPosition = AxisPosition{};
	metadata.clear();
}

std::size_t Toolpath::size() const {
	return commands.size();
}

bool Toolpath::empty() const {
	return commands.empty();
}

void Toolpath::setCurrentPosition(const AxisPosition& position) {
	currentPosition = position;
}

const AxisPosition& Toolpath::getCurrentPosition() const {
	return currentPosition;
}

void Toolpath::addCommand(const ToolpathCommand& command) {
	commands.push_back(command);
	currentPosition = command.end;
}

void Toolpath::addRapid(const AxisPosition& end, int sourceLine, const std::string& sourceText) {
	ToolpathCommand command;
	command.type = MotionType::Rapid;
	command.start = currentPosition;
	command.end = end;
	command.sourceLine = sourceLine;
	command.sourceText = sourceText;
	addCommand(command);
}

void Toolpath::addLinear(const AxisPosition& end, double feedRate, int sourceLine, const std::string& sourceText) {
	ToolpathCommand command;
	command.type = MotionType::Linear;
	command.start = currentPosition;
	command.end = end;
	command.feedRate = feedRate;
	command.sourceLine = sourceLine;
	command.sourceText = sourceText;
	addCommand(command);
}

void Toolpath::addArc(const AxisPosition& end, const AxisPosition& center, bool clockwise,
	ArcPlane plane, double feedRate, int sourceLine, const std::string& sourceText) {
	ToolpathCommand command;
	command.type = clockwise ? MotionType::ArcClockwise : MotionType::ArcCounterClockwise;
	command.start = currentPosition;
	command.end = end;
	command.center = center;
	command.plane = plane;
	command.feedRate = feedRate;
	command.sourceLine = sourceLine;
	command.sourceText = sourceText;
	addCommand(command);
}

void Toolpath::addDwell(double seconds, int sourceLine, const std::string& sourceText) {
	ToolpathCommand command;
	command.type = MotionType::Dwell;
	command.start = currentPosition;
	command.end = currentPosition;
	command.dwellSeconds = seconds;
	command.sourceLine = sourceLine;
	command.sourceText = sourceText;
	addCommand(command);
}

void Toolpath::addToolChange(int toolNumber, int sourceLine, const std::string& sourceText) {
	ToolpathCommand command;
	command.type = MotionType::ToolChange;
	command.start = currentPosition;
	command.end = currentPosition;
	command.toolNumber = toolNumber;
	command.sourceLine = sourceLine;
	command.sourceText = sourceText;
	addCommand(command);
}

void Toolpath::addSpindle(double speed, int sourceLine, const std::string& sourceText) {
	ToolpathCommand command;
	command.type = MotionType::Spindle;
	command.start = currentPosition;
	command.end = currentPosition;
	command.spindleSpeed = speed;
	command.sourceLine = sourceLine;
	command.sourceText = sourceText;
	addCommand(command);
}

ToolpathCommand Toolpath::getCommand(std::size_t index) const {
	if (index >= commands.size()) {
		throw std::out_of_range("Toolpath command index out of range");
	}
	return commands[index];
}

void Toolpath::setMetadata(const std::string& key, const std::string& value) {
	metadata[key] = value;
}

std::string Toolpath::getMetadata(const std::string& key) const {
	const auto found = metadata.find(key);
	return found == metadata.end() ? std::string{} : found->second;
}

} // namespace nc