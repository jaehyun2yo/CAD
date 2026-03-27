#pragma once

#include "ui/tool.h"
#include <vector>
#include <unordered_set>

namespace yjcad {

/// Stretch tool — selects endpoints inside a box and moves only those endpoints
/// Phase 1: Click first corner of selection box
/// Phase 2: Click second corner → defines stretch region
/// Phase 3: Click base point
/// Phase 4: Click destination → executes stretch
class StretchTool : public Tool {
public:
    std::string name() const override { return "스트레치"; }
    ToolMode mode() const override { return ToolMode::Stretch; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void executeStretch(const Point2D& destination);

    int m_phase = 0; // 0=box corner1, 1=box corner2, 2=base, 3=destination
    Point2D m_boxCorner1;
    Point2D m_boxCorner2;
    Point2D m_basePoint;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
