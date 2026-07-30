#pragma once

#include "toolpath/Toolpath.h"

namespace nc {

class MotionInterpolator {
public:
	static AxisPosition sample(const ToolpathCommand& command, double t);
	static double durationSeconds(
		const ToolpathCommand& command,
		double rapidFeedRate,
		double defaultFeedRate);
};

} // namespace nc
