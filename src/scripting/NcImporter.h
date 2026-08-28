#pragma once

#include "ofxSol2.h"
#include "toolpath/Toolpath.h"

#include <atomic>
#include <string>

namespace nc {

struct NcImportResult {
	bool success = false;
	Toolpath toolpath;
	std::string error;
};

NcImportResult importNcWithMachineScript(
	sol::state& state,
	const std::string& scriptPath,
	const std::string& ncText,
	std::atomic<float>* progress = nullptr,
	bool scriptPathRelativeToData = true);

} // namespace nc