#include "ofApp.h"

#include "simulation/MotionInterpolator.h"

#include "ofxGpr.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <fstream>

namespace {

bool tokenizeGCodeLine(
	const char* begin,
	const char* end,
	const char*& tokenBegin,
	const char*& tokenEnd,
	TextEditor::PaletteIndex& color) {
	if (begin == end) {
		return false;
	}

	color = TextEditor::PaletteIndex::Punctuation; // White for ordinary lines.
	bool insideParenthesisComment = false;
	for (const char* cursor = begin; cursor < end; ++cursor) {
		if (*cursor == ';' && !insideParenthesisComment) {
			break;
		}
		if (*cursor == '(') {
			insideParenthesisComment = true;
			continue;
		}
		if (*cursor == ')') {
			insideParenthesisComment = false;
			continue;
		}
		if (insideParenthesisComment || std::toupper(static_cast<unsigned char>(*cursor)) != 'G') {
			continue;
		}
		if (cursor > begin && std::isalpha(static_cast<unsigned char>(cursor[-1]))) {
			continue;
		}

		const char* digit = cursor + 1;
		if (digit >= end || !std::isdigit(static_cast<unsigned char>(*digit))) {
			continue;
		}

		int code = 0;
		while (digit < end && std::isdigit(static_cast<unsigned char>(*digit))) {
			code = code * 10 + (*digit - '0');
			++digit;
		}

		if (code == 0) {
			color = TextEditor::PaletteIndex::String; // Red.
			break;
		}
		if (code == 1) {
			color = TextEditor::PaletteIndex::Number; // Light green.
			break;
		}
		if (code == 2 || code == 3) {
			color = TextEditor::PaletteIndex::Keyword; // Light blue.
			break;
		}
	}

	tokenBegin = begin;
	tokenEnd = end;
	return true;
}

std::string compactGCode(std::string text) {
	text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char character) {
		return std::isspace(character) != 0;
	}), text.end());
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
		return static_cast<char>(std::toupper(character));
	});
	return text;
}

std::string canonicalGprBlock(const std::string& line) {
	if (line.empty()) {
		return {};
	}

	try {
		std::string terminatedLine = line;
		if (terminatedLine.back() != '\n') {
			terminatedLine.push_back('\n');
		}
		gpr::gcode_program program = gpr::parse_gcode_saving_block_text(terminatedLine);
		if (program.num_blocks() == 0) {
			return {};
		}
		return compactGCode(program.get_block(0).to_string());
	} catch (const std::exception&) {
		return {};
	}
}

double simulationSpeedFromSlider(float sliderValue) {
	const double magnitude = std::abs(static_cast<double>(sliderValue));
	const double speedMagnitude = magnitude <= 1.0
		? magnitude
		: 1.0 + (std::min(magnitude, 2.0) - 1.0) * 7.0;
	return std::copysign(speedMagnitude, static_cast<double>(sliderValue));
}

float simulationSliderFromSpeed(double speed) {
	const double magnitude = std::abs(speed);
	const double sliderMagnitude = magnitude <= 1.0
		? magnitude
		: 1.0 + (std::min(magnitude, 8.0) - 1.0) / 7.0;
	return static_cast<float>(std::copysign(sliderMagnitude, speed));
}
} // namespace

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetVerticalSync(true);
	ofDisableArbTex(); 

	gui.setup();

	const std::string initialNc =
		"; NC programi\n"
		"G21\n"
		"G90\n"
		"G0 X0 Y0 Z10\n"
		"G0 X50 Y50 Z10 F800\n"
		"G1 Z-5\n"
		"G1 X150\n"
		"G1 Y150\n"
		"G2 X200 Y200 I50 J0\n"
		"G3 X250 Y250 I0 J50\n"
		"G1 Y300\n"
		"G1 X50\n"
		"G1 Y50\n"
		"G0 Z10\n"
		"M30";

	scripting.setup();
	nc::lua::registerAllBindings(scripting.state());

	nc::MachineScriptFiles machineScripts;
	machineScripts.directory = "machines/generic_3axis";
	currentMachine.setScriptFiles(machineScripts);
	currentMachine.addLink("x_axis");
	currentMachine.addLink("y_axis", "x_axis");
	currentMachine.addLink("z_axis", "y_axis");
	scripting.state().set_function("of_log", [](const std::string& message) {
		ofLogNotice("Lua") << message;
	});

	TextEditor::LanguageDefinition gcodeLanguage = TextEditor::LanguageDefinition::GCode();
	gcodeLanguage.mTokenize = &tokenizeGCodeLine;
	gcodeLanguage.mTokenRegexStrings.clear();
	textEditor.SetLanguageDefinition(gcodeLanguage);
	textEditor.SetImGuiChildIgnored(true);
	textEditor.SetText(initialNc);
	interpretEditorText();

	
	//camera.setPosition(glm::vec3(100.0f, -100.0f, 50.0f));
	//camera.setTarget(glm::vec3(0.0f));
	//camera.setUpAxis(glm::vec3(0.0f, 0.0f, 1.0f));
	camera.enableMouseInput();
	resetCamera();
}

//--------------------------------------------------------------
void ofApp::interpretEditorText(){
	if (importInProgress) {
		return;
	}

	const std::string scriptPath = currentMachine.getImportScriptPath();
	const std::string ncText = textEditor.GetText();
	importInProgress = true;
	textEditor.SetReadOnly(true);
	statusMessage = "NC yorumlaniyor...";

	try {
		importFuture = std::async(std::launch::async, [this, scriptPath, ncText]() {
			return nc::importNcWithMachineScript(
				scripting.state(), scriptPath, ncText);
		});
	} catch (const std::exception& error) {
		importInProgress = false;
		textEditor.SetReadOnly(false);
		statusMessage = "Yorumlama baslatilamadi: " + std::string(error.what());
		ofLogError("NC Import") << statusMessage;
	}
}

//--------------------------------------------------------------
void ofApp::updateInterpretation(){
	if (!importInProgress || !importFuture.valid() ||
		importFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
		return;
	}

	nc::NcImportResult importResult;
	try {
		importResult = importFuture.get();
	} catch (const std::exception& error) {
		importResult.error = "NC yorumlama hatasi: " + std::string(error.what());
	} catch (...) {
		importResult.error = "NC yorumlama sirasinda bilinmeyen hata";
	}

	if (importResult.success) {
		currentToolpath = importResult.toolpath;
		rebuildToolpathEditorLines();
		rebuildToolpathMesh();
		simulation.setToolpath(&currentToolpath);
		simulation.play();
		editorSimulationLine = -1;
		statusMessage = "NC yorumlandi: " + ofToString(currentToolpath.size()) + " takim yolu komutu";
		ofLogNotice("NC Import") << statusMessage;
	} else {
		simulation.stop();
		toolpathMesh.clear();
		toolpathEditorLines.clear();
		editorSimulationLine = -1;
		statusMessage = importResult.error;
		ofLogError("NC Import") << importResult.error;
	}
	textEditor.SetReadOnly(false);
	importInProgress = false;
}
//--------------------------------------------------------------
void ofApp::rebuildToolpathEditorLines(){
	toolpathEditorLines.assign(currentToolpath.size(), -1);
	const std::vector<std::string> editorLines = textEditor.GetTextLines();
	std::vector<std::string> canonicalLines;
	canonicalLines.reserve(editorLines.size());
	for (const std::string& line : editorLines) {
		canonicalLines.push_back(canonicalGprBlock(line));
	}

	std::size_t searchStart = 0;
	std::string previousSource;
	int previousEditorLine = -1;
	for (std::size_t commandIndex = 0; commandIndex < currentToolpath.size(); ++commandIndex) {
		const nc::ToolpathCommand command = currentToolpath.getCommand(commandIndex);
		const std::string source = compactGCode(command.sourceText);
		if (source.empty()) {
			continue;
		}
		if (source == previousSource && previousEditorLine >= 0) {
			toolpathEditorLines[commandIndex] = previousEditorLine;
			continue;
		}

		for (std::size_t lineIndex = searchStart; lineIndex < canonicalLines.size(); ++lineIndex) {
			if (canonicalLines[lineIndex] == source) {
				previousSource = source;
				previousEditorLine = static_cast<int>(lineIndex);
				toolpathEditorLines[commandIndex] = previousEditorLine;
				searchStart = lineIndex + 1;
				break;
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::syncEditorToSimulation(){
	if (toolpathEditorLines.empty()) {
		return;
	}

	const std::size_t commandIndex = std::min(
		simulation.getCommandIndex(), toolpathEditorLines.size() - 1);
	const int editorLine = toolpathEditorLines[commandIndex];
	if (editorLine < 0) {
		return;
	}

	const bool lineChanged = editorLine != editorSimulationLine;
	const bool cursorMovedDuringPlayback =
		simulation.getStatus() == nc::SimulationStatus::Playing &&
		textEditor.GetCursorPosition().mLine != editorLine;
	if (!lineChanged && !cursorMovedDuringPlayback) {
		return;
	}

	const TextEditor::Coordinates position(editorLine, 0);
	textEditor.SetCursorPosition(position);
	textEditor.SetSelection(position, position);
	if (lineChanged) {
		editorSimulationLine = editorLine;
	}
}

//--------------------------------------------------------------
void ofApp::jumpSimulationToEditorLine(int editorLine){
	if (editorLine < 0 || simulation.getStatus() == nc::SimulationStatus::Playing) {
		return;
	}

	std::size_t movementOnLine = currentToolpath.size();
	std::size_t previousMovement = currentToolpath.size();
	std::size_t firstMovement = currentToolpath.size();
	for (std::size_t commandIndex = 0; commandIndex < currentToolpath.size(); ++commandIndex) {
		const nc::ToolpathCommand command = currentToolpath.getCommand(commandIndex);
		const bool isMovement =
			command.type == nc::MotionType::Rapid ||
			command.type == nc::MotionType::Linear ||
			command.type == nc::MotionType::ArcClockwise ||
			command.type == nc::MotionType::ArcCounterClockwise;
		if (!isMovement || commandIndex >= toolpathEditorLines.size()) {
			continue;
		}

		if (firstMovement == currentToolpath.size()) {
			firstMovement = commandIndex;
		}
		const int commandEditorLine = toolpathEditorLines[commandIndex];
		if (commandEditorLine == editorLine) {
			movementOnLine = commandIndex;
			break;
		}
		if (commandEditorLine >= 0 && commandEditorLine < editorLine) {
			previousMovement = commandIndex;
		}
	}

	std::size_t targetMovement = currentToolpath.size();
	double targetProgress = 0.0;
	if (movementOnLine < currentToolpath.size()) {
		targetMovement = movementOnLine;
		statusMessage = "Satir " + ofToString(editorLine + 1) + " takim yolu basina gidildi";
	} else if (previousMovement < currentToolpath.size()) {
		targetMovement = previousMovement;
		targetProgress = 1.0;
		statusMessage = "Onceki hareketin sonuna gidildi";
	} else if (firstMovement < currentToolpath.size()) {
		targetMovement = firstMovement;
		statusMessage = "Ilk takim yolu basina gidildi";
	}

	if (targetMovement < currentToolpath.size()) {
		simulation.seekToCommand(targetMovement, targetProgress);
		editorSimulationLine = toolpathEditorLines[targetMovement];
	}
}

//--------------------------------------------------------------
void ofApp::stepSimulationLine(int direction){
	if (direction == 0 || currentToolpath.empty() || toolpathEditorLines.empty()) {
		return;
	}

	std::vector<std::size_t> lineStarts;
	std::vector<std::size_t> lineEnds;
	int previousEditorLine = -1;
	for (std::size_t commandIndex = 0; commandIndex < currentToolpath.size(); ++commandIndex) {
		const nc::ToolpathCommand command = currentToolpath.getCommand(commandIndex);
		const bool isMovement =
			command.type == nc::MotionType::Rapid ||
			command.type == nc::MotionType::Linear ||
			command.type == nc::MotionType::ArcClockwise ||
			command.type == nc::MotionType::ArcCounterClockwise;
		if (!isMovement || commandIndex >= toolpathEditorLines.size()) {
			continue;
		}

		const int editorLine = toolpathEditorLines[commandIndex];
		if (editorLine < 0) {
			continue;
		}
		if (lineStarts.empty() || editorLine != previousEditorLine) {
			lineStarts.push_back(commandIndex);
			lineEnds.push_back(commandIndex);
			previousEditorLine = editorLine;
		} else {
			lineEnds.back() = commandIndex;
		}
	}
	if (lineStarts.empty()) {
		return;
	}

	const std::size_t commandIndex = simulation.getCommandIndex();
	std::size_t lineIndex = lineStarts.size() - 1;
	if (commandIndex < currentToolpath.size()) {
		for (std::size_t index = 0; index < lineStarts.size(); ++index) {
			if (lineStarts[index] > commandIndex) {
				lineIndex = index == 0 ? 0 : index - 1;
				break;
			}
		}
	}

	std::size_t targetCommand = lineStarts[lineIndex];
	double targetProgress = 0.0;
	if (direction > 0) {
		const bool currentLineFinished =
			commandIndex >= currentToolpath.size() ||
			(commandIndex == lineEnds[lineIndex] && simulation.getCommandProgress() >= 1.0);
		if (currentLineFinished && lineIndex + 1 < lineStarts.size()) {
			++lineIndex;
		}
		targetCommand = lineEnds[lineIndex];
		targetProgress = 1.0;
	} else {
		const bool atCurrentLineStart =
			commandIndex == lineStarts[lineIndex] && simulation.getCommandProgress() <= 0.0;
		if (atCurrentLineStart && lineIndex > 0) {
			--lineIndex;
		}
		targetCommand = lineStarts[lineIndex];
	}

	simulation.seekToCommand(targetCommand, targetProgress);
	editorSimulationLine = toolpathEditorLines[targetCommand];
	if (editorSimulationLine >= 0) {
		const TextEditor::Coordinates position(editorSimulationLine, 0);
		textEditor.SetCursorPosition(position);
		textEditor.SetSelection(position, position);
	}
	statusMessage = direction > 0 ? "Bir satir ileri gidildi" : "Bir satir geri gidildi";
}
//--------------------------------------------------------------
void ofApp::drawTextEditor(){
	const ImGuiWindowFlags editorFlags =
		ImGuiWindowFlags_HorizontalScrollbar |
		ImGuiWindowFlags_AlwaysHorizontalScrollbar |
		ImGuiWindowFlags_NoMove;
	ImGui::BeginChild("##GCodeEditor", ImVec2(0.0f, 0.0f), false, editorFlags);
	const ImVec2 editorMin = ImGui::GetWindowPos();
	const ImVec2 editorSize = ImGui::GetWindowSize();
	const ImVec2 editorMax(editorMin.x + editorSize.x, editorMin.y + editorSize.y);
	textEditor.Render("##GCodeEditorContent");
	const ImVec2 mousePosition = ImGui::GetIO().MousePos;
	const bool mouseInsideEditor =
		mousePosition.x >= editorMin.x && mousePosition.x < editorMax.x &&
		mousePosition.y >= editorMin.y && mousePosition.y < editorMax.y;
	if (simulation.getStatus() != nc::SimulationStatus::Playing &&
		mouseInsideEditor &&
		ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		jumpSimulationToEditorLine(textEditor.GetCursorPosition().mLine);
	}

	if (simulation.getStatus() == nc::SimulationStatus::Playing &&
		editorSimulationLine >= 0) {
		const TextEditor::Coordinates coordinates(editorSimulationLine, 0);
		textEditor.SetCursorPosition(coordinates);
	}

	ImGui::EndChild();
}

//--------------------------------------------------------------
void ofApp::saveEditorText(){
	std::ofstream output(ofToDataPath("program.nc"));
	if (output.is_open()) {
		output << textEditor.GetText();
	}
}

//--------------------------------------------------------------
void ofApp::rebuildToolpathMesh(){
	toolpathMesh.clear();
	toolpathMesh.setMode(OF_PRIMITIVE_LINES);

	const float unitScale = currentToolpath.units == nc::Units::Inches ? 25.4f : 1.0f;
	const auto point = [unitScale](const nc::AxisPosition& position) {
		return glm::vec3(
			static_cast<float>(position.x) * unitScale,
			static_cast<float>(position.y) * unitScale,
			static_cast<float>(position.z) * unitScale);
	};
	const auto addSegment = [this, &point](
		const nc::AxisPosition& start,
		const nc::AxisPosition& end,
		const ofFloatColor& color) {
		toolpathMesh.addVertex(point(start));
		toolpathMesh.addColor(color);
		toolpathMesh.addVertex(point(end));
		toolpathMesh.addColor(color);
	};

	for (std::size_t index = 0; index < currentToolpath.size(); ++index) {
		const nc::ToolpathCommand command = currentToolpath.getCommand(index);
		if (command.type == nc::MotionType::Rapid) {
			if (showRapidToolpaths) {
				addSegment(command.start, command.end, ofFloatColor(0.58f, 0.60f, 0.64f));
			}
		} else if (command.type == nc::MotionType::Linear) {
			addSegment(command.start, command.end, ofFloatColor(0.20f, 1.0f, 0.35f));
		} else if (command.type == nc::MotionType::ArcClockwise ||
			command.type == nc::MotionType::ArcCounterClockwise) {
			constexpr int ArcSegments = 48;
			nc::AxisPosition previous = command.start;
			for (int segment = 1; segment <= ArcSegments; ++segment) {
				const double t = static_cast<double>(segment) / ArcSegments;
				const nc::AxisPosition current = nc::MotionInterpolator::sample(command, t);
				addSegment(previous, current, ofFloatColor(1.0f, 0.65f, 0.15f));
				previous = current;
			}
		}
	}
}
//--------------------------------------------------------------
void ofApp::resetCamera(){
	const glm::vec3 target(0.0f, 0.0f, 0.0f);
	const glm::vec3 zUp(0.0f, 0.0f, 1.0f);

	camera.reset();
	camera.setUpAxis(zUp);
	camera.setRelativeYAxis(false);
	camera.setPosition(500.0f, -500.0f, 400.0f);
	camera.setTarget(target);
	camera.lookAt(target, zUp);
	//camera.setDistance(750.0f);
}

//--------------------------------------------------------------
void ofApp::update(){
	updateInterpretation();
	simulation.update(ofGetLastFrameTime());

	const nc::AxisPosition& position = simulation.getPosition();
	const float unitScale = currentToolpath.units == nc::Units::Inches ? 25.4f : 1.0f;
	if (nc::MachineLink* xAxis = currentMachine.findLink("x_axis")) {
		xAxis->setPosition(static_cast<float>(position.x) * unitScale, 0.0f, 0.0f);
	}
	if (nc::MachineLink* yAxis = currentMachine.findLink("y_axis")) {
		yAxis->setPosition(0.0f, static_cast<float>(position.y) * unitScale, 0.0f);
	}
	if (nc::MachineLink* zAxis = currentMachine.findLink("z_axis")) {
		zAxis->setPosition(0.0f, 0.0f, static_cast<float>(position.z) * unitScale);
	}
}

//--------------------------------------------------------------
void ofApp::drawScene(int width, int height){
	width = std::max(width, 1);
	height = std::max(height, 1);

	if (!sceneFbo.isAllocated() ||
		static_cast<int>(sceneFbo.getWidth()) != width ||
		static_cast<int>(sceneFbo.getHeight()) != height) {
		ofFbo::Settings settings;
		settings.width = width;
		settings.height = height;
		settings.internalformat = GL_RGBA;
		settings.useDepth = true;
		settings.depthStencilAsTexture = false;
		sceneFbo.allocate(settings);
	}

	sceneFbo.begin();
	ofClear(25, 27, 32, 255);
	ofEnableDepthTest();

	camera.begin();
	ofSetColor(80);
	ofDrawGrid(100.0f, 10, true, false, false, true);
	ofSetColor(255);
	ofDrawAxis(150.0f);
	ofPushStyle();
	ofSetColor(255);
	ofSetLineWidth(2.5f);
	toolpathMesh.draw();
	ofPopStyle();
	ofSetColor(255, 140, 40);
	//ofDrawBox(0.0f, 0.0f, 0.0f, 80.0f);
	currentMachine.drawLinks();
	camera.end();

	ofDisableDepthTest();
	const nc::AxisPosition& toolPosition = simulation.getPosition();
	const std::string unitLabel = currentToolpath.units == nc::Units::Inches ? "in" : "mm";
	const std::string coordinateText =
		"Takim Ucu [" + unitLabel + "]\n" +
		"X  " + ofToString(toolPosition.x, 3) + "\n" +
		"Y  " + ofToString(toolPosition.y, 3) + "\n" +
		"Z  " + ofToString(toolPosition.z, 3);
	ofPushStyle();
	ofEnableAlphaBlending();
	ofSetColor(10, 12, 16, 210);
	ofDrawRectangle(10.0f, 10.0f, 145.0f, 72.0f);
	ofSetColor(235, 238, 242);
	ofDrawBitmapString(coordinateText, 18.0f, 27.0f);
	ofPopStyle();
	sceneFbo.end();
}

//--------------------------------------------------------------
void ofApp::drawInterface(){
	syncEditorToSimulation();

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	const ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
	ImGui::Begin("##MainWorkspace", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Dosya")) {
			if (ImGui::MenuItem("Yeni")) {
				textEditor.SetText("");
				statusMessage = "Yeni belge olusturuldu";
			}
			if (ImGui::MenuItem("Kaydet", "Ctrl+S")) {
				saveEditorText();
				statusMessage = "program.nc kaydedildi";
			}
			if (ImGui::MenuItem("Cikis")) {
				ofExit();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Duzen")) {
			ImGui::MenuItem("G-code metin editoru", nullptr, false, false);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Gorunum")) {
			if (ImGui::MenuItem("Kamerayi sifirla")) {
				resetCamera();

				statusMessage = "Kamera sifirlandi";
			}
			if (ImGui::MenuItem("Rapid takim yollarini goster", nullptr, &showRapidToolpaths)) {
				rebuildToolpathMesh();
				statusMessage = showRapidToolpaths
					? "Rapid takim yollari gosteriliyor"
					: "Rapid takim yollari gizlendi";
			}
			ImGui::EndMenu();
		}
		ImGui::SameLine();
		static char str0[128] = "Hello, world!";
		ImGui::InputText("Position", str0, IM_ARRAYSIZE(str0));
		ImGui::SameLine();
		if (ImGui::Button("Ucur")) {
			auto pos = std::stoi(str0);
			auto coordi = TextEditor::Coordinates(pos, 0);
			textEditor.SetCursorPosition(coordi);
		}
		ImGui::EndMenuBar();
	}

	const float splitterWidth = 7.0f;
	const float footerHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f + 2.0f;
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float panelsHeight = std::max(available.y - footerHeight, 1.0f);
	const float usableWidth = std::max(available.x - splitterWidth, 1.0f);
	float leftWidth = usableWidth * splitRatio;

	ImGui::BeginChild("##EditorPanel", ImVec2(leftWidth, panelsHeight), true);
	ImGui::TextUnformatted("G-code Editor");
	ImGui::SameLine();
	ImGui::BeginDisabled(importInProgress);
	if (ImGui::Button("Yeniden Yorumla")) {
		interpretEditorText();
	}
	ImGui::EndDisabled();
	if (importInProgress) {
		const float phase = std::fmod(ofGetElapsedTimef() * 0.8f, 2.0f);
		const float progress = phase <= 1.0f ? phase : 2.0f - phase;
		ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "NC yorumlaniyor...");
	}
	ImGui::Separator();
	drawTextEditor();
	ImGui::EndChild();

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::InvisibleButton("##Splitter", ImVec2(splitterWidth, panelsHeight));
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}
	if (ImGui::IsItemActive()) {
		leftWidth += ImGui::GetIO().MouseDelta.x;
		splitRatio = ofClamp(leftWidth / usableWidth, 0.2f, 0.8f);
	}

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::BeginChild("##ScenePanel", ImVec2(0.0f, panelsHeight), true,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::TextUnformatted("3B Sahne");
	ImGui::SameLine();
	const bool cameraIsOrtho = camera.getOrtho();
	if (ImGui::Button(cameraIsOrtho ? "Perspektif" : "Ortho")) {
		if (cameraIsOrtho) {
			camera.disableOrtho();
			statusMessage = "Kamera perspektif modunda";
		} else {
			camera.enableOrtho();
			statusMessage = "Kamera ortho modunda";
		}
	}
	ImGui::Separator();
	const ImVec2 imageSize = ImGui::GetContentRegionAvail();
	const ImTextureID textureId = (ImTextureID)(uintptr_t)sceneFbo.getTexture().getTextureData().textureID;
	ImGui::Image(textureId, imageSize, ImVec2(0, 0), ImVec2(1, 1));

	// ofEasyCam pencere koordinatlariyla calisir. FBO goruntusunun ImGui
	// icindeki kesin ekran alanini kameranin fare kontrol alani olarak kullan.
	const ImVec2 sceneMin = ImGui::GetItemRectMin();
	const ImVec2 sceneMax = ImGui::GetItemRectMax();
	camera.setControlArea(ofRectangle(
		sceneMin.x,
		sceneMin.y,
		sceneMax.x - sceneMin.x,
		sceneMax.y - sceneMin.y));
	ImGui::EndChild();

	ImGui::Separator();
	if (ImGui::Button("<<")) {
		stepSimulationLine(-1);
	}
	ImGui::SameLine();
	if (ImGui::Button("Oynat")) {
		const double speed = simulation.getSpeed();
		simulation.setSpeed(speed == 0.0 ? 1.0 : std::abs(speed));
		simulation.play();
	}
	ImGui::SameLine();
	if (ImGui::Button("Duraklat")) {
		simulation.pause();
	}
	ImGui::SameLine();
	if (ImGui::Button("Durdur")) {
		simulation.stop();
	}
	ImGui::SameLine();
	if (ImGui::Button(">>")) {
		stepSimulationLine(1);
	}
	ImGui::SameLine();
	float speedSlider = simulationSliderFromSpeed(simulation.getSpeed());
	ImGui::SetNextItemWidth(180.0f);
	if (ImGui::SliderFloat("Hiz##SimulationSpeed", &speedSlider, -2.0f, 2.0f, "%.2f")) {
		const double speed = simulationSpeedFromSlider(speedSlider);
		simulation.setSpeed(speed);
		if (speed == 0.0) {
			simulation.pause();
		} else {
			simulation.play();
		}
	}
	ImGui::SameLine();
	float timelinePercent = static_cast<float>(simulation.getProgress() * 100.0);
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::SliderFloat("##SimulationTimeline", &timelinePercent, 0.0f, 100.0f, "%.1f%%")) {
		simulation.seek(timelinePercent / 100.0f);
	}

	const nc::AxisPosition& simulatedPosition = simulation.getPosition();
	const char* simulationText = "Durdu";
	if (simulation.getStatus() == nc::SimulationStatus::Playing) simulationText = simulation.getSpeed() < 0.0 ? "Geri sariyor" : "Oynuyor";
	if (simulation.getStatus() == nc::SimulationStatus::Paused) simulationText = "Duraklatildi";
	if (simulation.getStatus() == nc::SimulationStatus::Finished) simulationText = "Tamamlandi";
	ImGui::Text("%s | Sim: %s | Satir: %d | X %.2f Y %.2f Z %.2f | FPS: %.1f",
		statusMessage.c_str(), simulationText, editorSimulationLine + 1,
		simulatedPosition.x, simulatedPosition.y, simulatedPosition.z, ofGetFrameRate());

	ImGui::End();
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(18);

	const int sceneWidth = std::max(static_cast<int>(ofGetWidth() * (1.0f - splitRatio)), 1);
	const int sceneHeight = std::max(ofGetHeight() - 75, 1);
	drawScene(sceneWidth, sceneHeight);

	gui.begin();
	drawInterface();
	gui.end();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if ((ofGetKeyPressed(OF_KEY_CONTROL) || ofGetKeyPressed(OF_KEY_COMMAND)) &&
		(key == 's' || key == 'S')) {
		saveEditorText();
		statusMessage = "program.nc kaydedildi";
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){}
void ofApp::mouseMoved(int x, int y){}
void ofApp::mouseDragged(int x, int y, int button){}
void ofApp::mousePressed(int x, int y, int button){}
void ofApp::mouseReleased(int x, int y, int button){}
void ofApp::mouseEntered(int x, int y){}
void ofApp::mouseExited(int x, int y){}
void ofApp::windowResized(int w, int h){}
void ofApp::gotMessage(ofMessage msg){}
void ofApp::dragEvent(ofDragInfo dragInfo){}
