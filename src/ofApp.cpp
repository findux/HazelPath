#include "ofApp.h"

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
		"G1 X50 Y20 Z-2 F800\n";

	scripting.setup();
	nc::lua::registerAllBindings(scripting.state());
	scripting.state().set_function("of_log", [](const std::string& message) {
		ofLogNotice("Lua") << message;
	});

	const nc::NcImportResult importResult = nc::importNcWithMachineScript(
		scripting.state(), "machines/generic_3axis.lua", initialNc);
	if (importResult.success) {
		currentToolpath = importResult.toolpath;
		statusMessage = "NC donusturuldu: " + ofToString(currentToolpath.size()) + " takim yolu komutu";
		ofLogNotice("NC Import") << statusMessage;
	} else {
		statusMessage = importResult.error;
		ofLogError("NC Import") << importResult.error;
	}

	ofxImGuiTextEditorSettings editorSettings;
	editorSettings.languageDefinition = TextEditor::LanguageDefinition::GCode();
	textEditor.setup(editorSettings);
	textEditor.set(initialNc);

	
	//camera.setPosition(glm::vec3(100.0f, -100.0f, 50.0f));
	//camera.setTarget(glm::vec3(0.0f));
	//camera.setUpAxis(glm::vec3(0.0f, 0.0f, 1.0f));
	camera.enableMouseInput();
	resetCamera();
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
	ofSetColor(255, 140, 40);
	ofDrawBox(0.0f, 0.0f, 0.0f, 80.0f);
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
	const float statusHeight = ImGui::GetFrameHeightWithSpacing();
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float panelsHeight = std::max(available.y - statusHeight, 1.0f);
	const float usableWidth = std::max(available.x - splitterWidth, 1.0f);
	float leftWidth = usableWidth * splitRatio;

	ImGui::BeginChild("##EditorPanel", ImVec2(leftWidth, panelsHeight), true);
	ImGui::TextUnformatted("G-code Editor");
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
	ImGui::Text("%s | FPS: %.1f | Editor: %.0f%% | Sahne: %.0f%%",
		statusMessage.c_str(), ofGetFrameRate(), splitRatio * 100.0f, (1.0f - splitRatio) * 100.0f);

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
