#pragma once

#include "SceneState.h"

#include <optional>
#include <string>
#include <vector>

class MeasurementState final : public SceneState {
public:
	const char* id() const override;
	bool drawToolbarButton(bool active) override;
	void onEnter(const SceneStateContext& context) override;
	void onExit(const SceneStateContext& context) override;
	void onToolpathChanged(const SceneStateContext& context) override;
	void drawWorld(const SceneStateContext& context) override;
	void drawOverlay(const SceneStateContext& context) override;
	bool keyPressed(const SceneStateContext& context, int key) override;
	bool mouseMoved(const SceneStateContext& context, int x, int y) override;
	bool mouseDragged(const SceneStateContext& context, int x, int y, int button) override;
	bool mousePressed(const SceneStateContext& context, int x, int y, int button) override;
	bool mouseReleased(const SceneStateContext& context, int x, int y, int button) override;
	bool blocksCameraInput(const SceneStateContext& context) const override;
	bool requestsExit() const override;

private:
	struct PathSegment {
		glm::vec3 worldStart{0.0f};
		glm::vec3 worldEnd{0.0f};
		std::size_t commandIndex = 0;
		double commandTStart = 0.0;
		double commandTEnd = 1.0;
	};

	struct ScreenSegment {
		glm::vec2 start{0.0f};
		glm::vec2 end{0.0f};
		bool visible = false;
	};

	struct SnapPoint {
		bool valid = false;
		nc::AxisPosition modelPoint;
		glm::vec3 worldPoint{0.0f};
		glm::vec2 screenPoint{0.0f};
		std::size_t commandIndex = 0;
		double commandT = 0.0;
		float pixelDistance = 0.0f;
	};

	void clearSelection();
	void markGeometryDirty();
	void ensureGeometry(const SceneStateContext& context);
	void rebuildGeometry(const SceneStateContext& context);
	void ensureProjection(const SceneStateContext& context);
	void rebuildProjection(const SceneStateContext& context, const glm::mat4& viewProjection);
	void refreshHover(const SceneStateContext& context);
	SnapPoint findSnap(const SceneStateContext& context, const glm::vec2& mouse) const;
	glm::vec2 screenPosition(const SceneStateContext& context, const glm::vec3& world, float* depth = nullptr) const;
	void drawMotionHighlight(const SceneStateContext& context, std::size_t commandIndex) const;
	void drawInfoPanel(const SceneStateContext& context, ImDrawList* drawList) const;
	std::vector<std::string> infoLines(const SceneStateContext& context) const;

	std::vector<PathSegment> pathSegments;
	std::vector<ScreenSegment> screenSegments;
	std::optional<SnapPoint> firstPoint;
	std::optional<SnapPoint> secondPoint;
	SnapPoint hoverPoint;
	glm::vec2 mousePosition{0.0f};
	glm::mat4 cachedViewProjection{1.0f};
	ofRectangle cachedScreenRect;
	ofRectangle cachedRenderViewport;
	bool geometryDirty = true;
	bool projectionValid = false;
	bool cachedRapidVisible = true;
	bool exitRequested = false;
	float unitScale = 1.0f;
};
