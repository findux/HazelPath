#include "SceneStateManager.h"

void SceneStateManager::setResources(ofEasyCam* camera, const nc::Toolpath* toolpath) {
	context.camera = camera;
	context.toolpath = toolpath;
}

void SceneStateManager::setScreenRect(const ofRectangle& rectangle) {
	context.screenRect = rectangle;
}

void SceneStateManager::setRenderViewport(const ofRectangle& viewport) {
	context.renderViewport = viewport;
}

void SceneStateManager::setRapidVisible(bool visible) {
	context.showRapidToolpaths = visible;
}

void SceneStateManager::addState(std::unique_ptr<SceneState> state) {
	if (state) {
		states.push_back(std::move(state));
	}
}

void SceneStateManager::notifyToolpathChanged() {
	for (const auto& state : states) {
		state->onToolpathChanged(context);
	}
}

void SceneStateManager::deactivate() {
	if (!activeState) {
		return;
	}
	activeState->onExit(context);
	activeState = nullptr;
}

void SceneStateManager::update() {
	if (activeState) {
		activeState->update(context);
		handleExitRequest();
	}
}

void SceneStateManager::drawToolbar() {
	for (std::size_t index = 0; index < states.size(); ++index) {
		SceneState* state = states[index].get();
		ImGui::PushID(state->id());
		const bool clicked = state->drawToolbarButton(state == activeState);
		ImGui::PopID();
		if (clicked) {
			toggle(state);
		}
		if (index + 1 < states.size()) {
			ImGui::SameLine();
		}
	}
}

void SceneStateManager::drawWorld() {
	if (activeState) {
		activeState->drawWorld(context);
	}
}

void SceneStateManager::drawOverlay() {
	if (activeState) {
		activeState->drawOverlay(context);
		handleExitRequest();
	}
}

bool SceneStateManager::keyPressed(int key) {
	if (!activeState) {
		return false;
	}
	const bool handled = activeState->keyPressed(context, key);
	handleExitRequest();
	return handled;
}

bool SceneStateManager::keyReleased(int key) {
	return activeState && activeState->keyReleased(context, key);
}

bool SceneStateManager::mouseMoved(int x, int y) {
	return activeState && activeState->mouseMoved(context, x, y);
}

bool SceneStateManager::mouseDragged(int x, int y, int button) {
	return activeState && activeState->mouseDragged(context, x, y, button);
}

bool SceneStateManager::mousePressed(int x, int y, int button) {
	return activeState && activeState->mousePressed(context, x, y, button);
}

bool SceneStateManager::mouseReleased(int x, int y, int button) {
	return activeState && activeState->mouseReleased(context, x, y, button);
}

bool SceneStateManager::blocksCameraInput() const {
	return activeState && activeState->blocksCameraInput(context);
}

void SceneStateManager::toggle(SceneState* state) {
	if (state == activeState) {
		deactivate();
		return;
	}
	deactivate();
	activeState = state;
	activeState->onEnter(context);
}

void SceneStateManager::handleExitRequest() {
	if (activeState && activeState->requestsExit()) {
		deactivate();
	}
}
