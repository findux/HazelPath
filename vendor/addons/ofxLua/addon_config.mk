meta:
	ADDON_NAME = ofxLua
	ADDON_DESCRIPTION = Lua 5.4.8 static runtime for openFrameworks
	ADDON_AUTHOR = Lua.org contributors; openFrameworks wrapper
	ADDON_TAGS = "lua" "scripting"
	ADDON_URL = https://www.lua.org/

common:
	ADDON_INCLUDES = src
	ADDON_INCLUDES += libs/lua/src
	ADDON_SOURCES_EXCLUDE = libs/lua/src/lua.c
	ADDON_SOURCES_EXCLUDE += libs/lua/src/luac.c
	ADDON_SOURCES_EXCLUDE += libs/lua/src/onelua.c