#pragma once

#include "ofMain.h"

#ifndef SOL_ALL_SAFETIES_ON
#define SOL_ALL_SAFETIES_ON 1
#endif
#include <sol/sol.hpp>

class ofxSol2 {
public:
	bool setup();
	bool runString(const std::string& script);
	bool runFile(const std::string& path, bool relativeToData = true);

	sol::state& state() { return lua; }
	const sol::state& state() const { return lua; }
	const std::string& getLastError() const { return lastError; }

private:
	bool storeResult(sol::protected_function_result result);

	sol::state lua;
	std::string lastError;
};