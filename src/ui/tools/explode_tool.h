#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Explode tool — decomposes selected PolylineEntity into individual LineEntity segments
class ExplodeTool : public Tool {
public:
    std::string name() const override { return "분해"; }
    ToolMode mode() const override { return ToolMode::Explode; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    /// Execute explode on selected polylines
    void executeExplode();

    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
