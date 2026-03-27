#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Chamfer tool — cuts corners between two line segments and connects with a straight line
/// Phase 1: Click first line near the corner end
/// Phase 2: Click second line near the corner end → executes chamfer
/// The chamfer distance (default 5 mm) can be adjusted with +/- keys.
class ChamferTool : public Tool {
public:
    std::string name() const override { return "모깎기"; }
    ToolMode mode() const override { return ToolMode::Chamfer; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    bool keyPress(int key) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void executeChamfer(EntityId id1, const Point2D& click1,
                        EntityId id2, const Point2D& click2);

    bool m_firstSelected = false;
    EntityId m_firstId = 0;
    Point2D m_firstClick;
    double m_distance = 5.0; // mm
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
