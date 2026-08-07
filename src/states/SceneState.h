#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "camera/OrbitCameraController.h"
#include "toolpath/Toolpath.h"

struct SceneStateContext {
	OrbitCameraController* camera = nullptr;
	const nc::Toolpath* toolpath = nullptr;
	ofRectangle screenRect;
	ofRectangle renderViewport;
	bool showRapidToolpaths = true;
};

class SceneState {
public:
	virtual ~SceneState() = default;
	virtual const char* id() const = 0;
	virtual bool drawToolbarButton(bool active) = 0;
	virtual void onEnter(const SceneStateContext&) {}
	virtual void onExit(const SceneStateContext&) {}
	virtual void onToolpathChanged(const SceneStateContext&) {}
	virtual void update(const SceneStateContext&) {}
	virtual void drawWorld(const SceneStateContext&) {}
	virtual void drawOverlay(const SceneStateContext&) {}
	virtual bool keyPressed(const SceneStateContext&, int) { return false; }
	virtual bool keyReleased(const SceneStateContext&, int) { return false; }
	virtual bool mouseMoved(const SceneStateContext&, int, int) { return false; }
	virtual bool mouseDragged(const SceneStateContext&, int, int, int) { return false; }
	virtual bool mousePressed(const SceneStateContext&, int, int, int) { return false; }
	virtual bool mouseReleased(const SceneStateContext&, int, int, int) { return false; }
	virtual bool blocksCameraInput(const SceneStateContext&) const { return false; }
	virtual bool requestsExit() const { return false; }
};
