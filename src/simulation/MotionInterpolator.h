#pragma once

#include "toolpath/Toolpath.h"

namespace nc {

class MotionInterpolator {
public:
	static AxisPosition sample(const ToolpathCommand& command, double t);
	static int arcSegmentCount(
		const ToolpathCommand& command,
		double unitScale,
		double chordTolerance = 0.25);
	static double durationSeconds(
		const ToolpathCommand& command,
		double rapidFeedRate,
		double defaultFeedRate);
};

} // namespace nc
