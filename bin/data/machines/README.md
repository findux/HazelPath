# NC machine import scripts

Each machine profile is a reverse post-processor. It receives controller-specific NC text and returns the application's machine-independent `Toolpath` model.

Required entry point:

```lua
function convert_nc(nc_text)
    local program = gpr.parse_with_source(nc_text)
    local path = Toolpath.new()
    -- inspect GprProgram/GprBlock/GprChunk and append commands
    return path
end
```

Available GPR types:

- `GprProgram`: `num_blocks()`, `get_block(index)`
- `GprBlock`: `size()`, `get_chunk(index)`, `source()`, line/deletion helpers
- `GprChunk`: `type()`, `word()`, `address()`, `comment()`
- `GprAddress`: `type()`, `number()`, integer/double helpers
- `gpr.parse(text)`, `gpr.parse_with_source(text)`, `gpr.lex_block(text)`

Available toolpath types:

- `Toolpath`, `ToolpathCommand`, `AxisPosition`
- `MotionType`, `ArcPlane`, `ToolpathUnits`
- rapid, linear, arc, dwell, tool-change and spindle commands
- source line/text and string metadata fields

Arrays from GPR use zero-based indexes. A converter must keep controller modal state itself (units, absolute/incremental mode, active plane, active motion, feed, spindle and tool state).
## Per-machine layout

Each machine owns a directory with three scripts:

```text
machines/<machine-id>/
  machine.lua  # metadata and future URDF declaration
  import.lua   # reverse post-processor: NC -> Toolpath
  export.lua   # post-processor: Toolpath -> NC
```

`MachineScriptFiles` stores these paths. The C++ `Machine` class also exposes placeholder APIs for URDF loading, link/joint creation, visual meshes and forward kinematics.
## generic_3x_pcni

ADekoCAM PCNI import reads LX/LY/LZ from PARAMETRI01 and stores them as
Toolpath metadata. Only CONTORNATURA01 supplies motion; label data and other
sections are skipped. The output uses CAM coordinates with the part surface at Z=0:

- X is absolute; Y is LY minus the PCNI Y value.
- L=PON starts an operation at X/Y and Z=-PRF. The first start establishes
  the initial position; later starts are rapid positioning commands.
- G0/G1 use absolute X/Y and reversed incremental ZI: Z = previous Z - ZI.
  Ordinary Z words are ignored.
- G4 with positive R restores a CAM counterclockwise arc; G5 with negative R
  restores a CAM clockwise arc. R is a minor-arc radius, not a signed major-arc
  selector. I/J tangent mode is rejected because the supplied post disables it.
- QUP=n L=PUP retracts rapidly to Z=+n and closes the operation.
- F and the initial QVF are converted from m/min to the Toolpath mm/min unit.

L=G99 drills at absolute X and LY-Y, feeds to Z=-PRF using QVF (m/min), then
retracts to the QUP height in the closing L=PUP/L=POFF of the drilling group.
If the group has no closing QUP, the preceding L=PUP height is used.
T selects the tool and QVR sets
spindle rpm. X/Y/PRF/T/QVF/QVR and a group or preceding QUP are required. PNEXT does
not add motion. A standalone PUP can establish the initial clearance height.

Other motion macros are not inferred. An unknown
macro containing X/Y/ZI/PRF raises an error instead of drawing a guessed path.
The exporter remains a placeholder.
