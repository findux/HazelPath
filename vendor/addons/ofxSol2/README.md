# ofxSol2

sol2 3.3.0 single-header bindings for the `ofxLua` static Lua 5.4.8 runtime.
Add only `ofxSol2` to `addons.make`; Project Generator resolves `ofxLua` through ADDON_DEPENDENCIES.

```cpp
#include "ofxSol2.h"
ofxSol2 scripting;
scripting.setup();
scripting.state()["value"] = 42;
scripting.runString("print(value)");
```