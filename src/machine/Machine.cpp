#include "Machine.h"

#include <utility>

namespace nc {

MachineLink::MachineLink(std::string nameValue)
	: name(std::move(nameValue)) {}

void MachineLink::setName(const std::string& value) { name = value; }
const std::string& MachineLink::getName() const { return name; }

void MachineLink::customDraw(const ofBaseRenderer* renderer) const {
	// Temporary marker; URDF visual geometry will replace it later.
	renderer->drawAxis(15.0f);
	renderer->drawBox(8.0f);
}

Machine::Machine(std::string idValue, std::string nameValue)
	: id(std::move(idValue)), name(std::move(nameValue)) {}

void Machine::setId(const std::string& value) { id = value; }
const std::string& Machine::getId() const { return id; }
void Machine::setName(const std::string& value) { name = value; }
const std::string& Machine::getName() const { return name; }

void Machine::setScriptFiles(const MachineScriptFiles& files) { scriptFiles = files; }
const MachineScriptFiles& Machine::getScriptFiles() const { return scriptFiles; }

std::string Machine::getDefinitionScriptPath() const {
	return scriptFiles.directory + "/" + scriptFiles.definition;
}

std::string Machine::getImportScriptPath() const {
	return scriptFiles.directory + "/" + scriptFiles.importer;
}

std::string Machine::getExportScriptPath() const {
	return scriptFiles.directory + "/" + scriptFiles.exporter;
}

bool Machine::loadUrdf(const std::string& path) {
	// TODO: Parse the URDF and create an ofNode-based link hierarchy.
	(void)path;
	return false;
}

void Machine::unloadUrdf() {
	// TODO: Release parsed URDF resources.
}

bool Machine::isUrdfLoaded() const { return urdfLoaded; }
const std::string& Machine::getUrdfPath() const { return urdfPath; }

MachineLink& Machine::addLink(const std::string& linkName, const std::string& parentName) {
	auto link = std::make_unique<MachineLink>(linkName);
	MachineLink* result = link.get();

	if (MachineLink* parent = findLink(parentName)) {
		result->setParent(*parent);
	} else {
		result->setParent(*this);
	}

	links.push_back(std::move(link));
	return *result;
}

MachineLink* Machine::findLink(const std::string& linkName) {
	for (const auto& link : links) {
		if (link->getName() == linkName) {
			return link.get();
		}
	}
	return nullptr;
}

const MachineLink* Machine::findLink(const std::string& linkName) const {
	for (const auto& link : links) {
		if (link->getName() == linkName) {
			return link.get();
		}
	}
	return nullptr;
}

void Machine::clearLinks() {
	links.clear();
}

std::size_t Machine::getLinkCount() const {
	return links.size();
}

void Machine::drawLinks() const {
	for (const auto& link : links) {
		link->draw();
	}
}

} // namespace nc