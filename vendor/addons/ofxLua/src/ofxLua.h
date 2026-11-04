#pragma once

#include "ofMain.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

class ofxLua {
public:
	ofxLua() = default;
	~ofxLua();

	ofxLua(const ofxLua&) = delete;
	ofxLua& operator=(const ofxLua&) = delete;

	bool setup(bool openStandardLibraries = true);
	void close();

	bool runString(const std::string& script, const std::string& chunkName = "ofxLua");
	bool runFile(const std::string& path, bool relativeToData = true);

	lua_State* state() { return luaState; }
	const lua_State* state() const { return luaState; }
	bool isReady() const { return luaState != nullptr; }
	const std::string& getLastError() const { return lastError; }

private:
	bool finishCall(int loadStatus);

	lua_State* luaState = nullptr;
	std::string lastError;
};