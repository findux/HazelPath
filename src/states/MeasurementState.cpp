#include "MeasurementState.h"

#include "simulation/MotionInterpolator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr float SnapDistancePixels = 14.0f;
constexpr int ArcSampleCount = 48;
constexpr double TwoPi = 6.28318530717958647692;

glm::vec3 worldPoint(const nc::AxisPosition& position, float scale) {
	return glm::vec3(
		static_cast<float>(position.x) * scale,
		static_cast<float>(position.y) * scale,
		static_cast<float>(position.z) * scale);
}

void planeCoordinates(
	const nc::AxisPosition& position,
	nc::ArcPlane plane,
	double& u,
	double& v,
	double& perpendicular) {
	switch (plane) {
	case nc::ArcPlane::XY:
		u = position.x;
		v = position.y;
		perpendicular = position.z;
		break;
	case nc::ArcPlane::XZ:
		u = position.x;
		v = position.z;
		perpendicular = position.y;
		break;
	case nc::ArcPlane::YZ:
		u = position.y;
		v = position.z;
		perpendicular = position.x;
		break;
	}
}

double linearLength(const nc::ToolpathCommand& command) {
	const double dx = command.end.x - command.start.x;
	const double dy = command.end.y - command.start.y;
	const double dz = command.end.z - command.start.z;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double arcSweep(const nc::ToolpathCommand& command) {
	double startU = 0.0;
	double startV = 0.0;
	double startPerpendicular = 0.0;
	double endU = 0.0;
	double endV = 0.0;
	double endPerpendicular = 0.0;
	double centerU = 0.0;
	double centerV = 0.0;
	double centerPerpendicular = 0.0;
	planeCoordinates(command.start, command.plane, startU, startV, startPerpendicular);
	planeCoordinates(command.end, command.plane, endU, endV, endPerpendicular);
	planeCoordinates(command.center, command.plane, centerU, centerV, centerPerpendicular);

	const double startAngle = std::atan2(startV - centerV, startU - centerU);
	const double endAngle = std::atan2(endV - centerV, endU - centerU);
	double sweep = endAngle - startAngle;
	if (command.type == nc::MotionType::ArcClockwise) {
		if (sweep >= 0.0) {
			sweep -= TwoPi;
		}
	} else if (sweep <= 0.0) {
		sweep += TwoPi;
	}
	return sweep;
}

double arcRadius(const nc::ToolpathCommand& command) {
	double startU = 0.0;
	double startV = 0.0;
	double startPerpendicular = 0.0;
	double centerU = 0.0;
	double centerV = 0.0;
	double centerPerpendicular = 0.0;
	planeCoordinates(command.start, command.plane, startU, startV, startPerpendicular);
	planeCoordinates(command.center, command.plane, centerU, centerV, centerPerpendicular);
	return std::hypot(startU - centerU, startV - centerV);
}

double arcLength(const nc::ToolpathCommand& command) {
	double startU = 0.0;
	double startV = 0.0;
	double startPerpendicular = 0.0;
	double endU = 0.0;
	double endV = 0.0;
	double endPerpendicular = 0.0;
	planeCoordinates(command.start, command.plane, startU, startV, startPerpendicular);
	planeCoordinates(command.end, command.plane, endU, endV, endPerpendicular);
	const double planarLength = arcRadius(command) * std::abs(arcSweep(command));
	return std::hypot(planarLength, endPerpendicular - startPerpendicular);
}

bool isArc(const nc::ToolpathCommand& command) {
	return command.type == nc::MotionType::ArcClockwise ||
		command.type == nc::MotionType::ArcCounterClockwise;
}

bool isMotion(const nc::ToolpathCommand& command) {
	return command.type == nc::MotionType::Rapid ||
		command.type == nc::MotionType::Linear || isArc(command);
}

bool matricesDiffer(const glm::mat4& a, const glm::mat4& b) {
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (std::abs(a[column][row] - b[column][row]) > 0.000001f) {
				return true;
			}
		}
	}
	return false;
}

bool rectanglesDiffer(const ofRectangle& a, const ofRectangle& b) {
	return a.x != b.x || a.y != b.y || a.width != b.width || a.height != b.height;
}

std::string positionText(const nc::AxisPosition& position) {
	return "X " + ofToString(position.x, 3) +
		"  Y " + ofToString(position.y, 3) +
		"  Z " + ofToString(position.z, 3);
}

} // namespace

const char* MeasurementState::id() const {
	return "measurement";
}

bool MeasurementState::drawToolbarButton(bool active) {
	ensureToolbarIcon();

	const ImVec2 size(32.0f, 24.0f);
	const bool clicked = ImGui::InvisibleButton("##Caliper", size);
	const bool hovered = ImGui::IsItemHovered();
	const ImVec2 minimum = ImGui::GetItemRectMin();
	const ImVec2 maximum = ImGui::GetItemRectMax();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 background = active
		? IM_COL32(155, 195, 230, 255)
		: (hovered ? IM_COL32(230, 234, 238, 255) : IM_COL32(205, 211, 217, 255));
	drawList->AddRectFilled(minimum, maximum, background, 3.0f);
	drawList->AddRect(minimum, maximum, ImGui::GetColorU32(ImGuiCol_Border), 3.0f);

	if (toolbarIcon.isAllocated()) {
		const float availableWidth = size.x - 6.0f;
		const float availableHeight = size.y - 4.0f;
		const float aspect = toolbarIcon.getWidth() / toolbarIcon.getHeight();
		const float imageWidth = std::min(availableWidth, availableHeight * aspect);
		const float imageHeight = imageWidth / aspect;
		const ImVec2 imageMinimum(
			minimum.x + (size.x - imageWidth) * 0.5f,
			minimum.y + (size.y - imageHeight) * 0.5f);
		const ImVec2 imageMaximum(imageMinimum.x + imageWidth, imageMinimum.y + imageHeight);
		const ImTextureID textureId =
			(ImTextureID)(uintptr_t)toolbarIcon.getTexture().getTextureData().textureID;
		drawList->AddImage(textureId, imageMinimum, imageMaximum);
	} else {
		drawList->AddText(
			ImVec2(minimum.x + 10.0f, minimum.y + 4.0f),
			IM_COL32(20, 20, 20, 255),
			"M");
	}

	if (hovered) {
		ImGui::SetTooltip("Olcum araci");
	}
	return clicked;
}

void MeasurementState::ensureToolbarIcon() {
	if (toolbarIconLoadAttempted) {
		return;
	}
	toolbarIconLoadAttempted = true;
	const std::string iconPath = ofFilePath::join(
		ofFilePath::getCurrentExeDir(),
		"../res/kumpas.png");
	if (!toolbarIcon.load(iconPath)) {
		ofLogWarning("MeasurementState") << "Kumpas ikonu yuklenemedi: " << iconPath;
	}
}

void MeasurementState::onEnter(const SceneStateContext& context) {
	exitRequested = false;
	clearSelection();
	ensureGeometry(context);
}

void MeasurementState::onExit(const SceneStateContext&) {
	clearSelection();
	hoverPoint = {};
	exitRequested = false;
}

void MeasurementState::onToolpathChanged(const SceneStateContext&) {
	markGeometryDirty();
	clearSelection();
}

void MeasurementState::drawWorld(const SceneStateContext& context) {
	ensureGeometry(context);
	ofPushStyle();
	ofSetLineWidth(3.0f);
	if (hoverPoint.valid) {
		drawMotionHighlight(context, hoverPoint.commandIndex);
	}
	if (firstPoint) {
		ofSetColor(255, 215, 70);
		ofDrawSphere(firstPoint->worldPoint, 4.0f);
	}
	if (secondPoint) {
		ofSetColor(80, 220, 255);
		ofDrawSphere(secondPoint->worldPoint, 4.0f);
	}
	if (firstPoint && secondPoint) {
		ofSetColor(255, 235, 90);
		ofDrawLine(firstPoint->worldPoint, secondPoint->worldPoint);
	}
	ofPopStyle();
}

void MeasurementState::drawOverlay(const SceneStateContext& context) {
	mousePosition = glm::vec2(ofGetMouseX(), ofGetMouseY());
	refreshHover(context);
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	if (hoverPoint.valid) {
		drawList->AddCircle(
			ImVec2(hoverPoint.screenPoint.x, hoverPoint.screenPoint.y),
			6.0f,
			IM_COL32(255, 190, 45, 255),
			16,
			2.0f);
	}
	if (firstPoint) {
		const glm::vec2 screen = screenPosition(context, firstPoint->worldPoint);
		drawList->AddCircleFilled(ImVec2(screen.x, screen.y), 4.0f, IM_COL32(255, 215, 70, 255));
	}
	if (secondPoint) {
		const glm::vec2 screen = screenPosition(context, secondPoint->worldPoint);
		drawList->AddCircleFilled(ImVec2(screen.x, screen.y), 4.0f, IM_COL32(80, 220, 255, 255));
	}
	drawInfoPanel(context, drawList);
}

bool MeasurementState::keyPressed(const SceneStateContext&, int key) {
	if (key == OF_KEY_ESC) {
		if (firstPoint || secondPoint) {
			clearSelection();
		} else {
			exitRequested = true;
		}
		return true;
	}
	if (key == OF_KEY_BACKSPACE || key == OF_KEY_DEL) {
		clearSelection();
		return true;
	}
	return false;
}

bool MeasurementState::mouseMoved(const SceneStateContext& context, int x, int y) {
	mousePosition = glm::vec2(x, y);
	refreshHover(context);
	return context.screenRect.inside(x, y);
}

bool MeasurementState::mouseDragged(const SceneStateContext& context, int x, int y, int) {
	mousePosition = glm::vec2(x, y);
	refreshHover(context);
	return context.screenRect.inside(x, y);
}

bool MeasurementState::mousePressed(const SceneStateContext& context, int x, int y, int button) {
	mousePosition = glm::vec2(x, y);
	if (!context.screenRect.inside(x, y)) {
		return false;
	}
	if (button == OF_MOUSE_BUTTON_RIGHT) {
		clearSelection();
		return true;
	}
	if (button != OF_MOUSE_BUTTON_LEFT) {
		return false;
	}
	refreshHover(context);
	if (!hoverPoint.valid) {
		return true;
	}
	if (!firstPoint || secondPoint) {
		firstPoint = hoverPoint;
		secondPoint.reset();
	} else {
		secondPoint = hoverPoint;
	}
	return true;
}

bool MeasurementState::mouseReleased(const SceneStateContext& context, int x, int y, int) {
	return context.screenRect.inside(x, y);
}

bool MeasurementState::blocksCameraInput(const SceneStateContext& context) const {
	return context.screenRect.inside(ofGetMouseX(), ofGetMouseY());
}

bool MeasurementState::requestsExit() const {
	return exitRequested;
}

void MeasurementState::clearSelection() {
	firstPoint.reset();
	secondPoint.reset();
}

void MeasurementState::markGeometryDirty() {
	geometryDirty = true;
	projectionValid = false;
}

void MeasurementState::ensureGeometry(const SceneStateContext& context) {
	if (cachedRapidVisible != context.showRapidToolpaths) {
		geometryDirty = true;
	}
	if (geometryDirty) {
		rebuildGeometry(context);
	}
}

void MeasurementState::rebuildGeometry(const SceneStateContext& context) {
	pathSegments.clear();
	screenSegments.clear();
	cachedRapidVisible = context.showRapidToolpaths;
	geometryDirty = false;
	projectionValid = false;
	if (!context.toolpath) {
		return;
	}

	unitScale = context.toolpath->units == nc::Units::Inches ? 25.4f : 1.0f;
	for (std::size_t commandIndex = 0; commandIndex < context.toolpath->size(); ++commandIndex) {
		const nc::ToolpathCommand command = context.toolpath->getCommand(commandIndex);
		if (!isMotion(command) ||
			(command.type == nc::MotionType::Rapid && !context.showRapidToolpaths)) {
			continue;
		}
		const int sampleCount = isArc(command) ? ArcSampleCount : 1;
		for (int sample = 0; sample < sampleCount; ++sample) {
			const double t0 = static_cast<double>(sample) / sampleCount;
			const double t1 = static_cast<double>(sample + 1) / sampleCount;
			const nc::AxisPosition start = nc::MotionInterpolator::sample(command, t0);
			const nc::AxisPosition end = nc::MotionInterpolator::sample(command, t1);
			pathSegments.push_back({
				worldPoint(start, unitScale),
				worldPoint(end, unitScale),
				commandIndex,
				t0,
				t1
			});
		}
	}
}

void MeasurementState::ensureProjection(const SceneStateContext& context) {
	ensureGeometry(context);
	if (!context.camera || context.renderViewport.width <= 0.0f ||
		context.renderViewport.height <= 0.0f || context.screenRect.width <= 0.0f ||
		context.screenRect.height <= 0.0f) {
		projectionValid = false;
		return;
	}
	const glm::mat4 viewProjection =
		context.camera->getModelViewProjectionMatrix(context.renderViewport);
	if (!projectionValid || matricesDiffer(viewProjection, cachedViewProjection) ||
		rectanglesDiffer(context.screenRect, cachedScreenRect) ||
		rectanglesDiffer(context.renderViewport, cachedRenderViewport)) {
		rebuildProjection(context, viewProjection);
	}
}

void MeasurementState::rebuildProjection(
	const SceneStateContext& context,
	const glm::mat4& viewProjection) {
	screenSegments.clear();
	screenSegments.reserve(pathSegments.size());
	for (const PathSegment& segment : pathSegments) {
		float startDepth = 0.0f;
		float endDepth = 0.0f;
		const glm::vec2 start = screenPosition(context, segment.worldStart, &startDepth);
		const glm::vec2 end = screenPosition(context, segment.worldEnd, &endDepth);
		const bool visible = startDepth >= -1.0f && startDepth <= 1.0f &&
			endDepth >= -1.0f && endDepth <= 1.0f;
		screenSegments.push_back({start, end, visible});
	}
	cachedViewProjection = viewProjection;
	cachedScreenRect = context.screenRect;
	cachedRenderViewport = context.renderViewport;
	projectionValid = true;
}

void MeasurementState::refreshHover(const SceneStateContext& context) {
	ensureProjection(context);
	hoverPoint = findSnap(context, mousePosition);
}

MeasurementState::SnapPoint MeasurementState::findSnap(
	const SceneStateContext& context,
	const glm::vec2& mouse) const {
	SnapPoint result;
	result.pixelDistance = std::numeric_limits<float>::max();
	if (!projectionValid || !context.toolpath || !context.screenRect.inside(mouse)) {
		return result;
	}

	for (std::size_t index = 0; index < pathSegments.size(); ++index) {
		const ScreenSegment& screen = screenSegments[index];
		if (!screen.visible) {
			continue;
		}

		const PathSegment& path = pathSegments[index];
		const auto testEndpoint = [&](const glm::vec2& endpoint, double commandT) {
			const float distance = glm::distance(mouse, endpoint);
			if (distance >= result.pixelDistance || distance > SnapDistancePixels) {
				return;
			}
			const nc::ToolpathCommand command = context.toolpath->getCommand(path.commandIndex);
			const nc::AxisPosition model = nc::MotionInterpolator::sample(command, commandT);
			result.valid = true;
			result.modelPoint = model;
			result.worldPoint = worldPoint(model, unitScale);
			result.screenPoint = endpoint;
			result.commandIndex = path.commandIndex;
			result.commandT = commandT;
			result.pixelDistance = distance;
		};

		if (path.commandTStart <= 0.0) {
			testEndpoint(screen.start, 0.0);
		}
		if (path.commandTEnd >= 1.0) {
			testEndpoint(screen.end, 1.0);
		}
	}
	return result;
}

glm::vec2 MeasurementState::screenPosition(
	const SceneStateContext& context,
	const glm::vec3& world,
	float* depth) const {
	const glm::vec3 rendered = context.camera->worldToScreen(world, context.renderViewport);
	if (depth) {
		*depth = rendered.z;
	}
	return glm::vec2(
		context.screenRect.x +
			(rendered.x - context.renderViewport.x) / context.renderViewport.width * context.screenRect.width,
		context.screenRect.y +
			(rendered.y - context.renderViewport.y) / context.renderViewport.height * context.screenRect.height);
}

void MeasurementState::drawMotionHighlight(
	const SceneStateContext& context,
	std::size_t commandIndex) const {
	if (!context.toolpath || commandIndex >= context.toolpath->size()) {
		return;
	}
	ofSetColor(255, 170, 45);
	for (const PathSegment& segment : pathSegments) {
		if (segment.commandIndex == commandIndex) {
			ofDrawLine(segment.worldStart, segment.worldEnd);
		}
	}
	const nc::ToolpathCommand command = context.toolpath->getCommand(commandIndex);
	if (isArc(command)) {
		const glm::vec3 center = worldPoint(command.center, unitScale);
		ofSetColor(255, 100, 60);
		ofDrawSphere(center, 3.5f);
		ofDrawLine(center - glm::vec3(6.0f, 0.0f, 0.0f), center + glm::vec3(6.0f, 0.0f, 0.0f));
		ofDrawLine(center - glm::vec3(0.0f, 6.0f, 0.0f), center + glm::vec3(0.0f, 6.0f, 0.0f));
		ofDrawLine(center - glm::vec3(0.0f, 0.0f, 6.0f), center + glm::vec3(0.0f, 0.0f, 6.0f));
	}
}

void MeasurementState::drawInfoPanel(
	const SceneStateContext& context,
	ImDrawList* drawList) const {
	const std::vector<std::string> lines = infoLines(context);
	if (lines.empty()) {
		return;
	}
	ImFont* font = ImGui::GetFont();
	const float fontSize = ImGui::GetFontSize();
	float width = 0.0f;
	for (const std::string& line : lines) {
		width = std::max(width, font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line.c_str()).x);
	}
	const float padding = 8.0f;
	const float lineHeight = fontSize + 3.0f;
	const float height = padding * 2.0f + lineHeight * lines.size();
	const ImVec2 minimum(
		context.screenRect.x + 8.0f,
		context.screenRect.y + context.screenRect.height - height - 8.0f);
	const ImVec2 maximum(minimum.x + width + padding * 2.0f, minimum.y + height);
	drawList->AddRectFilled(minimum, maximum, IM_COL32(12, 15, 20, 225), 4.0f);
	drawList->AddRect(minimum, maximum, IM_COL32(90, 105, 120, 255), 4.0f);
	float y = minimum.y + padding;
	for (const std::string& line : lines) {
		drawList->AddText(font, fontSize, ImVec2(minimum.x + padding, y), IM_COL32(235, 240, 245, 255), line.c_str());
		y += lineHeight;
	}
}

std::vector<std::string> MeasurementState::infoLines(const SceneStateContext& context) const {
	const std::string unit = context.toolpath && context.toolpath->units == nc::Units::Inches ? "in" : "mm";
	std::vector<std::string> lines{
		"Olcum araci | Sol tik: sec | Sag tik: temizle | Esc: cik"
	};
	if (firstPoint) {
		lines.push_back("P1: " + positionText(firstPoint->modelPoint));
	}
	if (secondPoint) {
		lines.push_back("P2: " + positionText(secondPoint->modelPoint));
	}
	if (firstPoint && secondPoint) {
		const double dx = secondPoint->modelPoint.x - firstPoint->modelPoint.x;
		const double dy = secondPoint->modelPoint.y - firstPoint->modelPoint.y;
		const double dz = secondPoint->modelPoint.z - firstPoint->modelPoint.z;
		const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
		lines.push_back("Mesafe: " + ofToString(distance, 4) + " " + unit);
		lines.push_back(
			"Delta: X " + ofToString(dx, 4) +
			"  Y " + ofToString(dy, 4) +
			"  Z " + ofToString(dz, 4));
	}

	const SnapPoint* motionPoint = hoverPoint.valid
		? &hoverPoint
		: (secondPoint ? &*secondPoint : (firstPoint ? &*firstPoint : nullptr));
	if (!motionPoint || !context.toolpath || motionPoint->commandIndex >= context.toolpath->size()) {
		return lines;
	}
	const nc::ToolpathCommand command = context.toolpath->getCommand(motionPoint->commandIndex);
	if (command.type == nc::MotionType::Rapid || command.type == nc::MotionType::Linear) {
		lines.push_back(command.type == nc::MotionType::Rapid ? "Hareket: G0 Rapid" : "Hareket: G1 Dogrusal");
		lines.push_back("Uzunluk: " + ofToString(linearLength(command), 4) + " " + unit);
	} else if (isArc(command)) {
		lines.push_back(command.type == nc::MotionType::ArcClockwise ? "Hareket: G2 CW Yay" : "Hareket: G3 CCW Yay");
		lines.push_back("Yay uzunlugu: " + ofToString(arcLength(command), 4) + " " + unit);
		lines.push_back("Radius: " + ofToString(arcRadius(command), 4) + " " + unit);
		lines.push_back("Merkez: " + positionText(command.center));
	}
	if (command.sourceLine >= 0) {
		lines.push_back("NC satiri: " + ofToString(command.sourceLine + 1));
	}
	return lines;
}
