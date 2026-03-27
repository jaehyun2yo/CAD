#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Mirror tool — mirrors selected entities across a 2-click axis
/// Click 1: axis start, Click 2: axis end
class MirrorTool : public Tool {
public:
    std::string name() const override { return "미러"; }
    ToolMode mode() const override { return ToolMode::Mirror; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    bool m_hasFirst = false;
    Point2D m_axisStart;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
