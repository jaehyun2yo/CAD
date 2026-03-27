#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Offset tool — offset selected polyline/line by clicking direction
/// Click 1: select entity (if not already selected)
/// Click 2: direction + distance (perpendicular offset)
class OffsetTool : public Tool {
public:
    std::string name() const override { return "오프셋"; }
    ToolMode mode() const override { return ToolMode::Offset; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    bool m_entitySelected = false;
    EntityId m_targetId = 0;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
