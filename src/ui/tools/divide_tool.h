#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Divide tool — inserts N-1 point markers (small cross LineEntities) along a line
/// to divide it into N equal segments.
/// Click: select line entity. Key +/- to change count. Enter to commit.
/// Default N = 2 (one midpoint marker).
class DivideTool : public Tool {
public:
    std::string name() const override { return "분할"; }
    ToolMode mode() const override { return ToolMode::Divide; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    bool keyPress(int key) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void executeDivide();
    void buildPreview();

    bool m_lineSelected = false;
    EntityId m_targetId = 0;
    int m_divisions = 2; // number of equal segments (markers = divisions - 1)
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
