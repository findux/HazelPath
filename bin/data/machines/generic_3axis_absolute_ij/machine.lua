-- Machine metadata. URDF and kinematic fields will be consumed later.
return {
    id = "generic_3axis_absolute_ij",
    name = "Generic 3-Axis Mill (Absolute I/J)",
    urdf = "",
    axes = { "X", "Y", "Z" },
    scripts = {
        importer = "import.lua",
        exporter = "export.lua"
    }
}