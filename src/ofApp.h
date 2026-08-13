#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "ofxImGuiTextEditor.h"
#include "ofxSol2.h"
#include "scripting/LuaBindings.h"
#include "scripting/NcImporter.h"
#include "simulation/SimulationController.h"
#include "machine/Machine.h"
#include "camera/OrbitCameraController.h"
#include "states/SceneStateManager.h"

#include <future>

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
		void interpretEditorText();
		void updateInterpretation();
		void rebuildToolpathMesh();
		void rebuildToolpathEditorLines();
		void syncEditorToSimulation();
		void jumpSimulationToEditorLine(int editorLine);
		void stepSimulationLine(int direction);
		void drawTextEditor();
		void openEditorFile();
		bool saveEditorText();
		void drawScene(int width, int height);
		void drawInterface();

		ofxImGui::Gui gui;
		TextEditor textEditor;
		ofxSol2 scripting;
		nc::Toolpath currentToolpath;
		nc::Machine currentMachine{"generic_3axis", "Generic 3-Axis Mill"};
		nc::SimulationController simulation;
		SceneStateManager sceneStates;
		OrbitCameraController camera;
		ofFbo sceneFbo;
		ofVboMesh cuttingToolpathMesh;
		ofVboMesh rapidToolpathMesh;
		std::vector<int> toolpathEditorLines;
		std::future<nc::NcImportResult> importFuture;
		ofRectangle viewCubeRect;

		float splitRatio = 0.2f;
		bool showRapidToolpaths = true;
		bool importInProgress = false;
		bool viewCubeUsing = false;
		int editorSimulationLine = -1;
		int previousEditorCursorLine = -1;
		bool editorDocumentDirty = false;
		std::string editorFilePath;
		std::string savedEditorText;
		std::string statusMessage = "Hazir";
};
