# Pinned addon sources

`addons/` contains the exact addon source snapshots used by HazelPath, including
ofxGpr and local adaptations. The release build copies these into the SDK's
`addons/` directory. No network checkout of a moving addon branch is performed.
Git commits and release tags pin the snapshots, including nested libraries.
Update these files deliberately and validate a clean release build afterwards.

Upstream references (snapshots may contain local adaptations):

| Addon | Source |
| --- | --- |
| ofxGpr | https://github.com/dillonhuff/gpr (our openFrameworks adaptation) |
| ofxEnTT | https://github.com/skypjack/entt (openFrameworks wrapper) |
| ofxImGui | https://github.com/findux/ofxImGui |
| ofxImGuiTextEditor | https://github.com/findux/ofxImGuiTextEditor |
| ofxImGuizmo | https://github.com/findux/ofxImGuizmo |
| ofxLua | https://www.lua.org/ (local Lua 5.4.8 wrapper) |
| ofxSol2 | https://github.com/ThePhD/sol2 (local wrapper) |

These components retain their existing licenses and embedded notices. The
application GPL does not replace third-party licenses. See `docs/licensing.md`.
