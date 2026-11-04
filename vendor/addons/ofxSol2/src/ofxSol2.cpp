#include "ofxSol2.h"

bool ofxSol2::setup() {
	lua.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::coroutine,
		sol::lib::string,
		sol::lib::os,
		sol::lib::math,
		sol::lib::table,
		sol::lib::io,
		sol::lib::utf8,
		sol::lib::debug);
	lastError.clear();
	return true;
}

bool ofxSol2::storeResult(sol::protected_function_result result) {
	if (!result.valid()) {
		sol::error error = result;
		lastError = error.what();
		return false;
	}
	lastError.clear();
	return true;
}

bool ofxSol2::runString(const std::string& script) {
	return storeResult(lua.safe_script(script, sol::script_pass_on_error));
}

bool ofxSol2::runFile(const std::string& path, bool relativeToData) {
	const std::string resolvedPath = relativeToData ? ofToDataPath(path, true) : path;
	return storeResult(lua.safe_script_file(resolvedPath, sol::script_pass_on_error));
}