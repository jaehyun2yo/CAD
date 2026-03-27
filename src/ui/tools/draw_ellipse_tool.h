#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Interactive ellipse drawing tool
/// 3-click: center → X-axis radius point → Y-axis radius point → PolylineEntity (64 segments, closed)
class DrawEllipseTool : public Tool {
public:
    std::string name() const override { return "타원"; }
    ToolMode mode() const override { return ToolMode::DrawEllipse; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    // Step 0: waiting for center
    // Step 1: center set, waiting for X-radius point
    // Step 2: X-radius set, waiting for Y-radius point
    int m_step = 0;
    Point2D m_center;
    Point2D m_xAxisPt;  // defines X semi-axis length and rotation angle
    Point2D m_currentPos;
    std::vector<std::pair<Point2D, Point2D>> m_preview;

    static constexpr int SEGS = 64;

    void buildEllipsePreview(const Point2D& center,
                              double rx, double ry, double angleDeg);
};

} // namespace yjcad
