#include "ofApp.h"

#include "simulation/MotionInterpolator.h"

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
		"G1 X50 Y50 Z10 F800\n"
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

	ofxImGuiTextEditorSettings editorSettings;
	editorSettings.languageDefinition = TextEditor::LanguageDefinition::GCode();
	textEditor.setup(editorSettings);
	textEditor.set(initialNc);
	interpretEditorText();

	
	//camera.setPosition(glm::vec3(100.0f, -100.0f, 50.0f));
	//camera.setTarget(glm::vec3(0.0f));
	//camera.setUpAxis(glm::vec3(0.0f, 0.0f, 1.0f));
	camera.enableMouseInput();
	resetCamera();
}

//--------------------------------------------------------------
void ofApp::interpretEditorText(){
	const nc::NcImportResult importResult = nc::importNcWithMachineScript(
		scripting.state(), currentMachine.getImportScriptPath(), textEditor.get());
	if (importResult.success) {
		currentToolpath = importResult.toolpath;
		rebuildToolpathMesh();
		simulation.setToolpath(&currentToolpath);
		simulation.play();
		statusMessage = "NC yorumlandi: " + ofToString(currentToolpath.size()) + " takim yolu komutu";
		ofLogNotice("NC Import") << statusMessage;
	} else {
		simulation.stop();
		toolpathMesh.clear();
		statusMessage = importResult.error;
		ofLogError("NC Import") << importResult.error;
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
			addSegment(command.start, command.end, ofFloatColor(0.20f, 0.60f, 1.0f));
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
	sceneFbo.end();
}

//--------------------------------------------------------------
void ofApp::drawInterface(){
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
				textEditor.set("");
				statusMessage = "Yeni belge olusturuldu";
			}
			if (ImGui::MenuItem("Kaydet", "Ctrl+S")) {
				textEditor.save("program.nc");
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
			ImGui::EndMenu();
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
	if (ImGui::Button("Yeniden Yorumla")) {
		interpretEditorText();
	}
	ImGui::Separator();
	textEditor.draw("##GCodeEditor");
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
	if (ImGui::Button("Oynat")) {
		if (simulation.getSpeed() == 0.0) simulation.setSpeed(1.0);
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
	int speedStep = static_cast<int>(simulation.getSpeed());
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::SliderInt("Hiz##SimulationSpeed", &speedStep, -8, 8, "%dx")) {
		simulation.setSpeed(static_cast<double>(speedStep));
		if (speedStep == 0) {
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
		statusMessage.c_str(), simulationText, simulation.getSourceLine(),
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
		textEditor.save("program.nc");
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
