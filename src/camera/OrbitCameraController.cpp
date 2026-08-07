#include "OrbitCameraController.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr float Epsilon = 0.000001f;
constexpr float TwoPi = glm::two_pi<float>();

glm::vec3 normalizedOr(const glm::vec3& value, const glm::vec3& fallback) {
	const float length = glm::length(value);
	return length > Epsilon ? value / length : fallback;
}

} // namespace

OrbitCameraController::OrbitCameraController() {
	setNearClip(0.1f);
	setFarClip(1000000.0f);
	setFov(60.0f);
	setView(
		glm::vec3(500.0f, -500.0f, 400.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));
}

OrbitCameraController::~OrbitCameraController() {
	disableMouseInput();
}

void OrbitCameraController::setView(
	const glm::vec3& position,
	const glm::vec3& target,
	const glm::vec3& upAxis) {
	worldUp = normalizedOr(upAxis, glm::vec3(0.0f, 0.0f, 1.0f));
	targetPoint = target;
	const glm::vec3 offset = position - targetPoint;
	perspectiveDistance = std::clamp(
		glm::length(offset),
		minimumDistance,
		maximumDistance);
	const glm::vec3 direction = normalizedOr(offset, fallbackViewDirection());

	setScale(1.0f);
	if (getOrtho()) {
		const float visibleHeight =
			2.0f * perspectiveDistance * std::tan(glm::radians(getFov()) * 0.5f);
		orthoZoom = std::clamp(
			viewportHeight() / std::max(visibleHeight, Epsilon),
			minimumOrthoZoom,
			maximumOrthoZoom);
		setGlobalPosition(targetPoint + direction * orthographicDistance);
	} else {
		setGlobalPosition(targetPoint + direction * perspectiveDistance);
	}
	updateOrientation();
}

void OrbitCameraController::setTarget(const glm::vec3& target) {
	targetPoint = target;
	if (!getOrtho()) {
		perspectiveDistance = std::clamp(
			getDistance(),
			minimumDistance,
			maximumDistance);
	}
	updateOrientation();
}

const glm::vec3& OrbitCameraController::getTarget() const {
	return targetPoint;
}

void OrbitCameraController::setUpAxis(const glm::vec3& upAxis) {
	worldUp = normalizedOr(upAxis, glm::vec3(0.0f, 0.0f, 1.0f));
	updateOrientation();
}

const glm::vec3& OrbitCameraController::getUpAxis() const {
	return worldUp;
}

void OrbitCameraController::setDistance(float distance) {
	distance = std::clamp(distance, minimumDistance, maximumDistance);
	const glm::vec3 direction = normalizedOr(
		getGlobalPosition() - targetPoint,
		fallbackViewDirection());
	if (getOrtho()) {
		orthographicDistance = distance;
	} else {
		perspectiveDistance = distance;
	}
	setGlobalPosition(targetPoint + direction * distance);
	updateOrientation();
}

float OrbitCameraController::getDistance() const {
	return glm::distance(getGlobalPosition(), targetPoint);
}

void OrbitCameraController::setDistanceLimits(float minimum, float maximum) {
	minimumDistance = std::max(minimum, Epsilon);
	maximumDistance = std::max(maximum, minimumDistance);
	if (!getOrtho()) {
		setDistance(getDistance());
	}
}

void OrbitCameraController::setPolarAngleLimits(float minimum, float maximum) {
	const float epsilon = 0.001f;
	minimumPolarAngle = std::clamp(minimum, epsilon, glm::pi<float>() - epsilon);
	maximumPolarAngle = std::clamp(
		maximum,
		minimumPolarAngle,
		glm::pi<float>() - epsilon);
}

void OrbitCameraController::setRotateSpeed(float speed) {
	rotateSpeed = std::max(speed, 0.0f);
}

void OrbitCameraController::setPanSpeed(float speed) {
	panSpeed = std::max(speed, 0.0f);
}

void OrbitCameraController::setZoomSpeed(float speed) {
	zoomSpeed = std::max(speed, 0.0f);
}

void OrbitCameraController::setScreenSpacePanning(bool enabled) {
	screenSpacePanning = enabled;
}

void OrbitCameraController::setControlArea(const ofRectangle& area) {
	controlArea = area;
}

const ofRectangle& OrbitCameraController::getControlArea() const {
	return controlArea;
}

void OrbitCameraController::enableMouseInput() {
	if (mouseInputEnabled) {
		return;
	}
	mouseInputEnabled = true;
	mouseListeners.push(
		ofEvents().mousePressed.newListener(this, &OrbitCameraController::mousePressed));
	mouseListeners.push(
		ofEvents().mouseDragged.newListener(this, &OrbitCameraController::mouseDragged));
	mouseListeners.push(
		ofEvents().mouseReleased.newListener(this, &OrbitCameraController::mouseReleased));
	mouseListeners.push(
		ofEvents().mouseScrolled.newListener(this, &OrbitCameraController::mouseScrolled));
}

void OrbitCameraController::disableMouseInput() {
	if (!mouseInputEnabled) {
		return;
	}
	mouseInputEnabled = false;
	interaction = Interaction::None;
	mouseListeners.unsubscribeAll();
}

bool OrbitCameraController::getMouseInputEnabled() const {
	return mouseInputEnabled;
}

void OrbitCameraController::enableOrtho() {
	if (getOrtho()) {
		return;
	}
	perspectiveDistance = std::clamp(
		getDistance(),
		minimumDistance,
		maximumDistance);
	const float visibleHeight =
		2.0f * perspectiveDistance * std::tan(glm::radians(getFov()) * 0.5f);
	orthoZoom = std::clamp(
		viewportHeight() / std::max(visibleHeight, Epsilon),
		minimumOrthoZoom,
		maximumOrthoZoom);
	const glm::vec3 direction = normalizedOr(
		getGlobalPosition() - targetPoint,
		fallbackViewDirection());
	ofCamera::enableOrtho();
	setScale(1.0f);
	setGlobalPosition(targetPoint + direction * orthographicDistance);
	updateOrientation();
}

void OrbitCameraController::disableOrtho() {
	if (!getOrtho()) {
		return;
	}
	const glm::vec3 direction = normalizedOr(
		getGlobalPosition() - targetPoint,
		fallbackViewDirection());
	ofCamera::disableOrtho();
	setScale(1.0f);
	setGlobalPosition(targetPoint + direction * perspectiveDistance);
	updateOrientation();
}

float OrbitCameraController::getOrthoZoom() const {
	return orthoZoom;
}

void OrbitCameraController::setOrthoZoom(float zoom) {
	orthoZoom = std::clamp(zoom, minimumOrthoZoom, maximumOrthoZoom);
}

void OrbitCameraController::setOrthoZoomLimits(float minimum, float maximum) {
	minimumOrthoZoom = std::max(minimum, Epsilon);
	maximumOrthoZoom = std::max(maximum, minimumOrthoZoom);
	setOrthoZoom(orthoZoom);
}

void OrbitCameraController::setOrthographicDistance(float distance) {
	orthographicDistance = std::max(distance, minimumDistance);
	if (getOrtho()) {
		setDistance(orthographicDistance);
	}
}

void OrbitCameraController::begin() {
	begin(getViewport());
}

void OrbitCameraController::begin(const ofRectangle& viewport) {
	ofCamera::begin(viewport);
	if (getOrtho()) {
		ofSetMatrixMode(OF_MATRIX_PROJECTION);
		ofLoadMatrix(getProjectionMatrix(viewport));
		ofSetMatrixMode(OF_MATRIX_MODELVIEW);
	}
}

glm::mat4 OrbitCameraController::getProjectionMatrix(
	const ofRectangle& viewport) const {
	if (!getOrtho()) {
		return ofCamera::getProjectionMatrix(viewport);
	}
	const float zoom = std::max(orthoZoom, Epsilon);
	const float halfWidth = viewport.width * 0.5f / zoom;
	const float halfHeight = viewport.height * 0.5f / zoom;
	return glm::translate(
		glm::mat4(1.0f),
		glm::vec3(-getLensOffset().x, -getLensOffset().y, 0.0f)) *
		glm::ortho(
			-halfWidth,
			halfWidth,
			-halfHeight,
			halfHeight,
			getNearClip(),
			getFarClip());
}

glm::mat4 OrbitCameraController::getModelViewProjectionMatrix(
	const ofRectangle& viewport) const {
	return getProjectionMatrix(viewport) * getModelViewMatrix();
}

glm::vec3 OrbitCameraController::worldToScreen(
	const glm::vec3& world,
	const ofRectangle& viewport) const {
	glm::vec4 clip = getModelViewProjectionMatrix(viewport) * glm::vec4(world, 1.0f);
	if (std::abs(clip.w) <= Epsilon) {
		return glm::vec3(0.0f);
	}
	glm::vec3 normalizedDevice = glm::vec3(clip) / clip.w;
	if (isVFlipped()) {
		normalizedDevice.y = -normalizedDevice.y;
	}
	return glm::vec3(
		(normalizedDevice.x + 1.0f) * 0.5f * viewport.width + viewport.x,
		(1.0f - normalizedDevice.y) * 0.5f * viewport.height + viewport.y,
		normalizedDevice.z);
}

glm::vec3 OrbitCameraController::screenToWorld(
	const glm::vec3& screen,
	const ofRectangle& viewport) const {
	glm::vec3 normalizedDevice(
		2.0f * (screen.x - viewport.x) / viewport.width - 1.0f,
		1.0f - 2.0f * (screen.y - viewport.y) / viewport.height,
		screen.z);
	if (isVFlipped()) {
		normalizedDevice.y = -normalizedDevice.y;
	}
	glm::vec4 world = glm::inverse(getModelViewProjectionMatrix(viewport)) *
		glm::vec4(normalizedDevice, 1.0f);
	return std::abs(world.w) > Epsilon
		? glm::vec3(world) / world.w
		: glm::vec3(world);
}

void OrbitCameraController::mousePressed(ofMouseEventArgs& event) {
	if (!mouseInputEnabled || !controlArea.inside(event.x, event.y)) {
		return;
	}
	previousMouse = glm::vec2(event.x, event.y);
	if (event.button == OF_MOUSE_BUTTON_LEFT) {
		const bool panModifier =
			ofGetKeyPressed(OF_KEY_CONTROL) ||
			ofGetKeyPressed(OF_KEY_SHIFT) ||
			ofGetKeyPressed(OF_KEY_ALT);
		interaction = panModifier ? Interaction::Pan : Interaction::Rotate;
	} else if (event.button == OF_MOUSE_BUTTON_MIDDLE) {
		interaction = Interaction::Dolly;
	} else if (event.button == OF_MOUSE_BUTTON_RIGHT) {
		interaction = Interaction::Pan;
	}
}

void OrbitCameraController::mouseDragged(ofMouseEventArgs& event) {
	if (!mouseInputEnabled || interaction == Interaction::None) {
		return;
	}
	const glm::vec2 mouse(event.x, event.y);
	const glm::vec2 delta = mouse - previousMouse;
	previousMouse = mouse;
	switch (interaction) {
	case Interaction::Rotate:
		orbit(delta);
		break;
	case Interaction::Pan:
		pan(delta);
		break;
	case Interaction::Dolly:
		dolly(delta.y);
		break;
	default:
		break;
	}
}

void OrbitCameraController::mouseReleased(ofMouseEventArgs&) {
	interaction = Interaction::None;
}

void OrbitCameraController::mouseScrolled(ofMouseEventArgs& event) {
	if (!mouseInputEnabled || !controlArea.inside(event.x, event.y)) {
		return;
	}
	dolly(-event.scrollY * 10.0f);
}

void OrbitCameraController::orbit(const glm::vec2& delta) {
	const glm::vec3 up = safeUpAxis();
	const glm::vec3 reference =
		std::abs(up.z) < 0.999f
		? glm::vec3(0.0f, 0.0f, 1.0f)
		: glm::vec3(0.0f, 1.0f, 0.0f);
	const glm::vec3 basisX = normalizedOr(glm::cross(reference, up), glm::vec3(1.0f, 0.0f, 0.0f));
	const glm::vec3 basisY = glm::normalize(glm::cross(up, basisX));
	const glm::vec3 offset = getGlobalPosition() - targetPoint;
	const float radius = std::max(glm::length(offset), minimumDistance);
	const glm::vec3 direction = normalizedOr(offset, fallbackViewDirection());

	float polar = std::acos(std::clamp(glm::dot(direction, up), -1.0f, 1.0f));
	float azimuth = std::atan2(
		glm::dot(direction, basisY),
		glm::dot(direction, basisX));
	const float height = viewportHeight();
	azimuth -= delta.x * TwoPi * rotateSpeed / height;
	polar -= delta.y * TwoPi * rotateSpeed / height;
	polar = std::clamp(polar, minimumPolarAngle, maximumPolarAngle);

	const float sinPolar = std::sin(polar);
	const glm::vec3 newOffset =
		basisX * (radius * sinPolar * std::cos(azimuth)) +
		basisY * (radius * sinPolar * std::sin(azimuth)) +
		up * (radius * std::cos(polar));
	setGlobalPosition(targetPoint + newOffset);
	updateOrientation();
}

void OrbitCameraController::pan(const glm::vec2& delta) {
	const float height = viewportHeight();
	float worldPerPixel = 1.0f / std::max(orthoZoom, Epsilon);
	if (!getOrtho()) {
		worldPerPixel =
			2.0f * getDistance() * std::tan(glm::radians(getFov()) * 0.5f) / height;
	}
	worldPerPixel *= panSpeed;

	const glm::vec3 right = normalizedOr(
		getGlobalOrientation() * glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 0.0f, 0.0f));
	const glm::vec3 vertical = screenSpacePanning
		? normalizedOr(
			getGlobalOrientation() * glm::vec3(0.0f, 1.0f, 0.0f),
			safeUpAxis())
		: safeUpAxis();
	const glm::vec3 movement =
		-right * delta.x * worldPerPixel +
		vertical * delta.y * worldPerPixel;
	targetPoint += movement;
	setGlobalPosition(getGlobalPosition() + movement);
}

void OrbitCameraController::dolly(float delta) {
	const float scale = std::exp(delta * 0.01f * zoomSpeed);
	if (getOrtho()) {
		setOrthoZoom(orthoZoom / scale);
	} else {
		setDistance(getDistance() * scale);
	}
}

void OrbitCameraController::updateOrientation() {
	const glm::vec3 viewDirection = normalizedOr(
		targetPoint - getGlobalPosition(),
		-fallbackViewDirection());
	glm::vec3 viewUp = safeUpAxis();
	if (std::abs(glm::dot(viewDirection, viewUp)) > 0.999f) {
		const glm::vec3 reference =
			std::abs(viewDirection.y) < 0.9f
			? glm::vec3(0.0f, 1.0f, 0.0f)
			: glm::vec3(1.0f, 0.0f, 0.0f);
		viewUp = normalizedOr(
			reference - viewDirection * glm::dot(reference, viewDirection),
			glm::vec3(0.0f, 1.0f, 0.0f));
	}
	lookAt(targetPoint, viewUp);
}

glm::vec3 OrbitCameraController::safeUpAxis() const {
	return normalizedOr(worldUp, glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::vec3 OrbitCameraController::fallbackViewDirection() const {
	const glm::vec3 up = safeUpAxis();
	const glm::vec3 reference =
		std::abs(up.z) < 0.999f
		? glm::vec3(0.0f, 0.0f, 1.0f)
		: glm::vec3(0.0f, 1.0f, 0.0f);
	const glm::vec3 basisX = normalizedOr(
		glm::cross(reference, up),
		glm::vec3(1.0f, 0.0f, 0.0f));
	const glm::vec3 basisY = glm::normalize(glm::cross(up, basisX));
	return glm::normalize(basisX - basisY + up * 0.75f);
}

float OrbitCameraController::viewportHeight() const {
	if (controlArea.height > Epsilon) {
		return controlArea.height;
	}
	return static_cast<float>(std::max(ofGetHeight(), 1));
}
