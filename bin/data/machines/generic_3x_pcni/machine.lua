-- Machine metadata. URDF and kinematic fields will be consumed later.
return {
    id = "generic_3x_pcni",
    name = "generic_3x_pcni",
    urdf = "",
    axes = { "X", "Y", "Z" },
    scripts = {
        importer = "import.lua",
        exporter = "export.lua"
    }
}