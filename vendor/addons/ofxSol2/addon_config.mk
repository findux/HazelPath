meta:
	ADDON_NAME = ofxSol2
	ADDON_DESCRIPTION = sol2 3.3.0 single-header C++ bindings for Lua
	ADDON_AUTHOR = ThePhD and contributors; openFrameworks wrapper
	ADDON_TAGS = "lua" "sol2" "scripting"
	ADDON_URL = https://github.com/ThePhD/sol2

common:
	ADDON_DEPENDENCIES = ofxLua
	ADDON_INCLUDES = src
	ADDON_INCLUDES += libs/sol2/include