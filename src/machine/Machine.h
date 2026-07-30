#pragma once

#include "ofMain.h"

#include <memory>
#include <string>
#include <vector>

namespace nc {

struct MachineScriptFiles {
	std::string directory;
	std::string definition = "machine.lua";
	std::string importer = "import.lua";
	std::string exporter = "export.lua";
};

class MachineLink : public ofNode {
public:
	MachineLink() = default;
	explicit MachineLink(std::string name);

	void setName(const std::string& value);
	const std::string& getName() const;

protected:
	void customDraw(const ofBaseRenderer* renderer) const override;

private:
	std::string name;
};

class Machine : public ofNode {
public:
	Machine() = default;
	Machine(std::string id, std::string name);

	void setId(const std::string& value);
	const std::string& getId() const;
	void setName(const std::string& value);
	const std::string& getName() const;

	void setScriptFiles(const MachineScriptFiles& files);
	const MachineScriptFiles& getScriptFiles() const;
	std::string getDefinitionScriptPath() const;
	std::string getImportScriptPath() const;
	std::string getExportScriptPath() const;

	bool loadUrdf(const std::string& path);
	void unloadUrdf();
	bool isUrdfLoaded() const;
	const std::string& getUrdfPath() const;

	MachineLink& addLink(const std::string& name, const std::string& parentName = "");
	MachineLink* findLink(const std::string& name);
	const MachineLink* findLink(const std::string& name) const;
	void clearLinks();
	std::size_t getLinkCount() const;
	void drawLinks() const;

private:
	std::string id;
	std::string name;
	std::string urdfPath;
	MachineScriptFiles scriptFiles;
	std::vector<std::unique_ptr<MachineLink>> links;
	bool urdfLoaded = false;
};

} // namespace nc
