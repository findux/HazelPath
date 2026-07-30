#include "LuaBindings.h"
#include "machine/Machine.h"

namespace nc::lua {

void registerMachineBindings(sol::state& state) {
	state.new_usertype<MachineScriptFiles>("MachineScriptFiles",
		sol::constructors<MachineScriptFiles()>(),
		"directory", &MachineScriptFiles::directory,
		"definition", &MachineScriptFiles::definition,
		"importer", &MachineScriptFiles::importer,
		"exporter", &MachineScriptFiles::exporter);

	state.new_usertype<MachineLink>("MachineLink", sol::no_constructor,
		"set_name", &MachineLink::setName,
		"get_name", &MachineLink::getName,
		"set_position", [](MachineLink& link, float x, float y, float z) {
			link.setPosition(x, y, z);
		},
		"get_x", [](const MachineLink& link) { return link.getX(); },
		"get_y", [](const MachineLink& link) { return link.getY(); },
		"get_z", [](const MachineLink& link) { return link.getZ(); },
		"set_orientation", [](MachineLink& link, float x, float y, float z) {
			link.setOrientation(glm::vec3(x, y, z));
		},
		"set_scale", [](MachineLink& link, float x, float y, float z) {
			link.setScale(x, y, z);
		});

	state.new_usertype<Machine>("Machine",
		sol::constructors<Machine(), Machine(std::string, std::string)>(),
		"set_id", &Machine::setId,
		"get_id", &Machine::getId,
		"set_name", &Machine::setName,
		"get_name", &Machine::getName,
		"set_script_files", &Machine::setScriptFiles,
		"get_script_files", &Machine::getScriptFiles,
		"get_definition_script_path", &Machine::getDefinitionScriptPath,
		"get_import_script_path", &Machine::getImportScriptPath,
		"get_export_script_path", &Machine::getExportScriptPath,
		"load_urdf", &Machine::loadUrdf,
		"unload_urdf", &Machine::unloadUrdf,
		"is_urdf_loaded", &Machine::isUrdfLoaded,
		"get_urdf_path", &Machine::getUrdfPath,
		"add_link", sol::overload(
			[](Machine& machine, const std::string& name) -> MachineLink& {
				return machine.addLink(name);
			},
			[](Machine& machine, const std::string& name, const std::string& parentName) -> MachineLink& {
				return machine.addLink(name, parentName);
			}),
		"find_link", [](Machine& machine, const std::string& name) {
			return machine.findLink(name);
		},
		"clear_links", &Machine::clearLinks,
		"get_link_count", &Machine::getLinkCount,
		"draw_links", &Machine::drawLinks);
}

} // namespace nc::lua

