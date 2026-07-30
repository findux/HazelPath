-- Machine metadata. URDF and kinematic fields will be consumed later.
return {
    id = "generic_3axis",
    name = "Generic 3-Axis Mill",
    urdf = "",
    axes = { "X", "Y", "Z" },
    scripts = {
        importer = "import.lua",
        exporter = "export.lua"
    }
}