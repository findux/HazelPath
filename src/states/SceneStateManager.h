#pragma once

#include "SceneState.h"

#include <memory>
#include <vector>

class SceneStateManager {
public:
	void setResources(OrbitCameraController* camera, const nc::Toolpath* toolpath);
	void setScreenRect(const ofRectangle& rectangle);
	void setRenderViewport(const ofRectangle& viewport);
	void setRapidVisible(bool visible);

	void addState(std::unique_ptr<SceneState> state);
	void notifyToolpathChanged();
	void deactivate();

	void update();
	void drawToolbar();
	void drawWorld();
	void drawOverlay();

	bool keyPressed(int key);
	bool keyReleased(int key);
	bool mouseMoved(int x, int y);
	bool mouseDragged(int x, int y, int button);
	bool mousePressed(int x, int y, int button);
	bool mouseReleased(int x, int y, int button);
	bool blocksCameraInput() const;

private:
	void toggle(SceneState* state);
	void handleExitRequest();

	SceneStateContext context;
	std::vector<std::unique_ptr<SceneState>> states;
	SceneState* activeState = nullptr;
};
