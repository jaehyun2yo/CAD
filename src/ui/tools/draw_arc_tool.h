#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Interactive arc drawing tool
/// 3-click: center → start point → end point (CCW arc)
class DrawArcTool : public Tool {
public:
    std::string name() const override { return "호 그리기"; }
    ToolMode mode() const override { return ToolMode::DrawArc; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void buildArcPreview(const Point2D& center, double radius,
                         double startDeg, double endDeg);

    int m_clickCount = 0;
    Point2D m_center;
    Point2D m_startPoint;
    double m_radius = 0.0;
    double m_startAngleDeg = 0.0;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
