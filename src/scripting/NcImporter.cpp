#include "NcImporter.h"

#include "ofMain.h"

namespace nc {

NcImportResult importNcWithMachineScript(
	sol::state& state,
	const std::string& scriptPath,
	const std::string& ncText,
	std::atomic<float>* progress,
	bool scriptPathRelativeToData) {
	NcImportResult output;
	if (progress) {
		progress->store(0.0f);
		state.set_function("of_progress", [progress](int current, int total) {
			if (total > 0) progress->store(static_cast<float>(current) / total);
		});
	}
	const std::string resolvedPath = scriptPathRelativeToData
		? ofToDataPath(scriptPath, true)
		: scriptPath;

	state["convert_nc"] = sol::nil;
	const sol::protected_function_result loadResult =
		state.safe_script_file(resolvedPath, sol::script_pass_on_error);
	if (!loadResult.valid()) {
		const sol::error error = loadResult;
		output.error = "Makine betigi yuklenemedi: " + std::string(error.what());
		return output;
	}

	const sol::object converterObject = state["convert_nc"];
	if (!converterObject.valid() || converterObject.get_type() != sol::type::function) {
		output.error = "Makine betigi convert_nc(nc_text) fonksiyonunu tanimlamiyor";
		return output;
	}

	sol::protected_function converter = converterObject.as<sol::protected_function>();
	std::string terminatedNcText = ncText;
	if (terminatedNcText.empty() || terminatedNcText.back() != '\n') {
		terminatedNcText.push_back('\n');
	}
	const sol::protected_function_result conversionResult = converter(terminatedNcText);
	if (!conversionResult.valid()) {
		const sol::error error = conversionResult;
		output.error = "NC donusum hatasi: " + std::string(error.what());
		return output;
	}

	const sol::object resultObject = conversionResult.get<sol::object>();
	if (!resultObject.is<Toolpath>()) {
		output.error = "convert_nc bir Toolpath nesnesi dondurmedi";
		return output;
	}

	output.toolpath = resultObject.as<Toolpath>();
	if (progress) progress->store(1.0f);
	output.success = true;
	return output;
}

} // namespace nc