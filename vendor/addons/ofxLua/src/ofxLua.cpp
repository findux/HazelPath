#include "ofxLua.h"

ofxLua::~ofxLua() {
	close();
}

bool ofxLua::setup(bool openStandardLibraries) {
	close();
	luaState = luaL_newstate();
	if (!luaState) {
		lastError = "Lua state olusturulamadi";
		return false;
	}
	if (openStandardLibraries) {
		luaL_openlibs(luaState);
	}
	lastError.clear();
	return true;
}

void ofxLua::close() {
	if (luaState) {
		lua_close(luaState);
		luaState = nullptr;
	}
}

bool ofxLua::finishCall(int loadStatus) {
	if (!luaState) {
		lastError = "Lua setup() cagrilmadi";
		return false;
	}
	int status = loadStatus;
	if (status == LUA_OK) {
		status = lua_pcall(luaState, 0, LUA_MULTRET, 0);
	}
	if (status != LUA_OK) {
		const char* message = lua_tostring(luaState, -1);
		lastError = message ? message : "Bilinmeyen Lua hatasi";
		lua_pop(luaState, 1);
		return false;
	}
	lastError.clear();
	return true;
}

bool ofxLua::runString(const std::string& script, const std::string& chunkName) {
	if (!luaState) {
		lastError = "Lua setup() cagrilmadi";
		return false;
	}
	return finishCall(luaL_loadbuffer(luaState, script.data(), script.size(), chunkName.c_str()));
}

bool ofxLua::runFile(const std::string& path, bool relativeToData) {
	if (!luaState) {
		lastError = "Lua setup() cagrilmadi";
		return false;
	}
	const std::string resolvedPath = relativeToData ? ofToDataPath(path, true) : path;
	return finishCall(luaL_loadfile(luaState, resolvedPath.c_str()));
}