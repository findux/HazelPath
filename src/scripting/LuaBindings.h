#pragma once

#include "ofxSol2.h"

namespace nc::lua {

void registerToolpathBindings(sol::state& state);
void registerGprBindings(sol::state& state);
void registerAllBindings(sol::state& state);

} // namespace nc::lua