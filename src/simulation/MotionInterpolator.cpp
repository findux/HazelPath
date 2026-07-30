#include "MotionInterpolator.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double TwoPi = 6.28318530717958647692;

nc::AxisPosition lerpPosition(
	const nc::AxisPosition& start,
	const nc::AxisPosition& end,
	double t) {
	nc::AxisPosition result;
	result.x = start.x + (end.x - start.x) * t;
	result.y = start.y + (end.y - start.y) * t;
	result.z = start.z + (end.z - start.z) * t;
	result.a = start.a + (end.a - start.a) * t;
	result.b = start.b + (end.b - start.b) * t;
	result.c = start.c + (end.c - start.c) * t;
	return result;
}

void planeCoordinates(
	const nc::AxisPosition& position,
	nc::ArcPlane plane,
	double& u,
	double& v,
	double& perpendicular) {
	switch (plane) {
	case nc::ArcPlane::XY:
		u = position.x;
		v = position.y;
		perpendicular = position.z;
		break;
	case nc::ArcPlane::XZ:
		u = position.x;
		v = position.z;
		perpendicular = position.y;
		break;
	case nc::ArcPlane::YZ:
		u = position.y;
		v = position.z;
		perpendicular = position.x;
		break;
	}
}

void setPlaneCoordinates(
	nc::AxisPosition& position,
	nc::ArcPlane plane,
	double u,
	double v,
	double perpendicular) {
	switch (plane) {
	case nc::ArcPlane::XY:
		position.x = u;
		position.y = v;
		position.z = perpendicular;
		break;
	case nc::ArcPlane::XZ:
		position.x = u;
		position.z = v;
		position.y = perpendicular;
		break;
	case nc::ArcPlane::YZ:
		position.y = u;
		position.z = v;
		position.x = perpendicular;
		break;
	}
}

double arcSweep(const nc::ToolpathCommand& command) {
	double startU = 0.0;
	double startV = 0.0;
	double startPerpendicular = 0.0;
	double endU = 0.0;
	double endV = 0.0;
	double endPerpendicular = 0.0;
	double centerU = 0.0;
	double centerV = 0.0;
	double centerPerpendicular = 0.0;
	planeCoordinates(command.start, command.plane, startU, startV, startPerpendicular);
	planeCoordinates(command.end, command.plane, endU, endV, endPerpendicular);
	planeCoordinates(command.center, command.plane, centerU, centerV, centerPerpendicular);

	const double startAngle = std::atan2(startV - centerV, startU - centerU);
	const double endAngle = std::atan2(endV - centerV, endU - centerU);
	double sweep = endAngle - startAngle;

	if (command.type == nc::MotionType::ArcClockwise) {
		if (sweep >= 0.0) {
			sweep -= TwoPi;
		}
	} else if (sweep <= 0.0) {
		sweep += TwoPi;
	}
	return sweep;
}

double linearDistance(const nc::AxisPosition& start, const nc::AxisPosition& end) {
	const double dx = end.x - start.x;
	const double dy = end.y - start.y;
	const double dz = end.z - start.z;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double arcDistance(const nc::ToolpathCommand& command) {
	double startU = 0.0;
	double startV = 0.0;
	double startPerpendicular = 0.0;
	double endU = 0.0;
	double endV = 0.0;
	double endPerpendicular = 0.0;
	double centerU = 0.0;
	double centerV = 0.0;
	double centerPerpendicular = 0.0;
	planeCoordinates(command.start, command.plane, startU, startV, startPerpendicular);
	planeCoordinates(command.end, command.plane, endU, endV, endPerpendicular);
	planeCoordinates(command.center, command.plane, centerU, centerV, centerPerpendicular);

	const double radius = std::hypot(startU - centerU, startV - centerV);
	const double planarLength = radius * std::abs(arcSweep(command));
	return std::hypot(planarLength, endPerpendicular - startPerpendicular);
}

} // namespace

namespace nc {

AxisPosition MotionInterpolator::sample(const ToolpathCommand& command, double t) {
	t = std::max(0.0, std::min(t, 1.0));
	if (t <= 0.0) {
		return command.start;
	}
	if (t >= 1.0) {
		return command.end;
	}

	if (command.type != MotionType::ArcClockwise &&
		command.type != MotionType::ArcCounterClockwise) {
		return lerpPosition(command.start, command.end, t);
	}

	double startU = 0.0;
	double startV = 0.0;
	double startPerpendicular = 0.0;
	double endU = 0.0;
	double endV = 0.0;
	double endPerpendicular = 0.0;
	double centerU = 0.0;
	double centerV = 0.0;
	double centerPerpendicular = 0.0;
	planeCoordinates(command.start, command.plane, startU, startV, startPerpendicular);
	planeCoordinates(command.end, command.plane, endU, endV, endPerpendicular);
	planeCoordinates(command.center, command.plane, centerU, centerV, centerPerpendicular);

	const double radius = std::hypot(startU - centerU, startV - centerV);
	const double startAngle = std::atan2(startV - centerV, startU - centerU);
	const double angle = startAngle + arcSweep(command) * t;
	const double perpendicular = startPerpendicular +
		(endPerpendicular - startPerpendicular) * t;

	AxisPosition result = lerpPosition(command.start, command.end, t);
	setPlaneCoordinates(
		result,
		command.plane,
		centerU + std::cos(angle) * radius,
		centerV + std::sin(angle) * radius,
		perpendicular);
	return result;
}

double MotionInterpolator::durationSeconds(
	const ToolpathCommand& command,
	double rapidFeedRate,
	double defaultFeedRate) {
	if (command.type == MotionType::Dwell) {
		return std::max(command.dwellSeconds, 0.0);
	}

	double distance = 0.0;
	if (command.type == MotionType::Rapid || command.type == MotionType::Linear) {
		distance = linearDistance(command.start, command.end);
	} else if (command.type == MotionType::ArcClockwise ||
		command.type == MotionType::ArcCounterClockwise) {
		distance = arcDistance(command);
	} else {
		return 0.0;
	}

	const double feedRate = command.type == MotionType::Rapid
		? rapidFeedRate
		: (command.feedRate > 0.0 ? command.feedRate : defaultFeedRate);
	return feedRate > 0.0 ? distance * 60.0 / feedRate : 0.0;
}

} // namespace nc
