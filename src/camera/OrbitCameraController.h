#pragma once

#include "ofMain.h"

class OrbitCameraController final : public ofCamera {
public:
	OrbitCameraController();
	~OrbitCameraController() override;

	void setView(
		const glm::vec3& position,
		const glm::vec3& target,
		const glm::vec3& upAxis);
	void setTarget(const glm::vec3& target);
	const glm::vec3& getTarget() const;
	void setUpAxis(const glm::vec3& upAxis);
	const glm::vec3& getUpAxis() const;

	void setDistance(float distance);
	float getDistance() const;
	void setDistanceLimits(float minimum, float maximum);
	void setPolarAngleLimits(float minimum, float maximum);

	void setRotateSpeed(float speed);
	void setPanSpeed(float speed);
	void setZoomSpeed(float speed);
	void setScreenSpacePanning(bool enabled);

	void setControlArea(const ofRectangle& area);
	const ofRectangle& getControlArea() const;
	void enableMouseInput();
	void disableMouseInput();
	bool getMouseInputEnabled() const;

	void enableOrtho();
	void disableOrtho();
	float getOrthoZoom() const;
	void setOrthoZoom(float zoom);
	void setOrthoZoomLimits(float minimum, float maximum);
	void setOrthographicDistance(float distance);

	void begin() override;
	void begin(const ofRectangle& viewport) override;
	glm::mat4 getProjectionMatrix(const ofRectangle& viewport) const;
	glm::mat4 getModelViewProjectionMatrix(const ofRectangle& viewport) const;
	glm::vec3 worldToScreen(
		const glm::vec3& world,
		const ofRectangle& viewport) const;
	glm::vec3 screenToWorld(
		const glm::vec3& screen,
		const ofRectangle& viewport) const;

private:
	enum class Interaction {
		None,
		Rotate,
		Pan,
		Dolly
	};

	void mousePressed(ofMouseEventArgs& event);
	void mouseDragged(ofMouseEventArgs& event);
	void mouseReleased(ofMouseEventArgs& event);
	void mouseScrolled(ofMouseEventArgs& event);

	void orbit(const glm::vec2& delta);
	void pan(const glm::vec2& delta);
	void dolly(float delta);
	void updateOrientation();
	glm::vec3 safeUpAxis() const;
	glm::vec3 fallbackViewDirection() const;
	float viewportHeight() const;

	glm::vec3 targetPoint{0.0f};
	glm::vec3 worldUp{0.0f, 0.0f, 1.0f};
	ofRectangle controlArea;
	ofEventListeners mouseListeners;
	glm::vec2 previousMouse{0.0f};
	Interaction interaction = Interaction::None;

	float rotateSpeed = 1.0f;
	float panSpeed = 1.0f;
	float zoomSpeed = 1.0f;
	float minimumDistance = 0.1f;
	float maximumDistance = 10000000.0f;
	float minimumPolarAngle = 0.001f;
	float maximumPolarAngle = glm::pi<float>() - 0.001f;
	float orthoZoom = 1.0f;
	float minimumOrthoZoom = 0.0001f;
	float maximumOrthoZoom = 10000.0f;
	float perspectiveDistance = 1000.0f;
	float orthographicDistance = 5000.0f;
	bool screenSpacePanning = true;
	bool mouseInputEnabled = false;
};
