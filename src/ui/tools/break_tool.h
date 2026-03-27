#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Break tool — splits a line entity at a clicked point into two segments
/// Phase 1: Click to select the line to break (picks closest line)
/// Phase 2: Click the split point on the line
class BreakTool : public Tool {
public:
    std::string name() const override { return "브레이크"; }
    ToolMode mode() const override { return ToolMode::Break; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    bool m_lineSelected = false;
    EntityId m_targetId = 0;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
