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