#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace nc {

// Cumulative vertex counts at a command boundary: cutting, rapid.
using ToolpathVertexBoundary = std::array<std::size_t, 2>;

struct ToolpathDrawRange {
	std::size_t completedVertices = 0;
	double partialProgress = 0.0;
};

inline ToolpathDrawRange toolpathDrawRange(
	std::size_t begin, std::size_t end, double commandProgress) {
	const double progress = std::clamp(commandProgress, 0.0, 1.0);
	const double segments = static_cast<double>((end - begin) / 2) * progress;
	const auto completed = static_cast<std::size_t>(std::floor(segments));
	return {begin + completed * 2, segments - static_cast<double>(completed)};
}

} // namespace nc
