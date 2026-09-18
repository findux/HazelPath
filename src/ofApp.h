#pragma once

#include "ofMain.h"
#include "ofxEnTT.h"
#include "ofxImGui.h"
#include "ofxImGuiTextEditor.h"
#include "ofxSol2.h"
#include "scripting/LuaBindings.h"
#include "scripting/NcImporter.h"
#include "simulation/SimulationController.h"
#include "simulation/ToolpathVisibility.h"
#include "machine/Machine.h"
#include "camera/OrbitCameraController.h"
#include "states/SceneStateManager.h"

#include <array>
#include <atomic>
#include <future>
#include <memory>
#include <vector>

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

	private:
		void resetCamera();
		void fitCamera();
		void updateWindowTitle();
		nc::Machine& activeMachine();
		const nc::Machine& activeMachine() const;
		void interpretEditorText();
		void updateInterpretation();
		void reportImportError(const std::string& message);
		void drawImportErrorDialog();
		void rebuildToolpathMesh();
		void drawToolpaths();
		void rebuildToolpathEditorLines();
		void syncEditorToSimulation();
		void jumpSimulationToEditorLine(int editorLine);
		void jumpToToolpathCommand(std::size_t commandIndex);
		std::size_t findToolpathCommandAtScreenPoint(const glm::vec2& point) const;
		void stepSimulationLine(int direction);
		void playSimulation();
		void pauseSimulation();
		void stopSimulation();
		void drawTextEditor();
		void drawFindDialog();
		bool findInEditor(bool forward);
		void replaceCurrentFind();
		void replaceAllFind();
		void rememberFindQuery();
		bool loadEditorFile(const std::string& path);
		void openEditorFile();
		bool saveEditorText();
		void drawScene(int width, int height);
		void drawInterface();

		ofxImGui::Gui gui;
		TextEditor textEditor;
		ofxSol2 scripting;
		nc::Toolpath currentToolpath;
		std::vector<std::unique_ptr<nc::Machine>> machines;
		std::size_t activeMachineIndex = 0;
		nc::SimulationController simulation;
		entt::registry ecsRegistry;
		SceneStateManager sceneStates;
		OrbitCameraController camera;
		ofFbo sceneFbo;
		ofVboMesh cuttingToolpathMesh;
		ofVboMesh rapidToolpathMesh;
		std::vector<nc::ToolpathVertexBoundary> toolpathVertexBoundaries;
		bool simulationViewActive = false;
		std::vector<int> toolpathEditorLines;
		std::future<nc::NcImportResult> importFuture;
		ofRectangle viewCubeRect;
		ofRectangle sceneScreenRect;
		ofRectangle sceneRenderViewport;

		float splitRatio = 0.2f;
		std::array<char, 256> findQuery{};
		std::array<char, 256> replaceQuery{};
		std::vector<std::string> findHistory;
		std::string lastFindQuery;
		std::size_t findMatchOffset = std::string::npos;
		std::size_t findMatchLength = 0;
		bool findDialogOpen = false;
		bool findOnlyMode = false;
		bool findMatchCase = false;
		bool findWholeWord = false;
		bool findLoop = true;
		bool findRegex = false;

		bool showRapidToolpaths = true;
		std::atomic<float> importProgress{0.0f};
		bool importInProgress = false;
        bool importErrorDialogOpen = false;
        std::string importErrorReport;
        std::string importSourceText;
        std::string importSourceName;
        std::string importMachineName;
        int importErrorLine = -1;
		bool reinterpretationNeeded = false;
		bool viewCubeUsing = false;
		int editorSimulationLine = -1;
		int previousEditorCursorLine = -1;
		bool editorDocumentDirty = false;
		std::string editorFilePath;
		std::string currentWindowTitle;
		std::string statusMessage = "Hazir";
		glm::vec2 sceneMousePressPosition{0.0f};
		glm::vec2 lastSceneClickPosition{0.0f};
		std::uint64_t lastSceneClickMillis = 0;
};
