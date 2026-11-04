# ofxLua

Lua 5.4.8 is compiled directly into an openFrameworks project. No DLL is required.
`lua.c`, `luac.c`, and `onelua.c` are excluded because they are standalone entry points.

Add `ofxLua` to `addons.make`, then include `ofxLua.h`.