#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Scale tool — scales selected entities from a base point
/// Click 1: base point, Click 2: reference distance, Click 3: target distance
class ScaleTool : public Tool {
public:
    std::string name() const override { return "스케일"; }
    ToolMode mode() const override { return ToolMode::Scale; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    int m_clickCount = 0;
    Point2D m_basePoint;
    Point2D m_refPoint;
    double m_refDist = 0.0;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
