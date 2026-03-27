#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Interactive point drawing tool
/// 1-click: places a cross marker (two LineEntities: horizontal + vertical, 2mm each)
class DrawPointTool : public Tool {
public:
    std::string name() const override { return "점"; }
    ToolMode mode() const override { return ToolMode::DrawPoint; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override { (void)worldPos; return false; }
    void cancel() override {}

    // No rubber-band preview needed for single-click point tool
    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        static std::vector<std::pair<Point2D, Point2D>> empty;
        return empty;
    }

private:
    // Cross marker half-size in mm
    static constexpr double MARKER_HALF = 1.0; // total 2mm
};

} // namespace yjcad
