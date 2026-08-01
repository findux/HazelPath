#pragma once

#include "ofxSol2.h"

namespace nc::lua {

void registerGlmBindings(sol::state& state);
void registerToolpathBindings(sol::state& state);
void registerGprBindings(sol::state& state);
void registerMachineBindings(sol::state& state);
void registerAllBindings(sol::state& state);

} // namespace nc::lua
