#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Distance tool — click two points to measure distance, angle, DX, DY
class DistanceTool : public Tool {
public:
    std::string name() const override { return "거리측정"; }
    ToolMode mode() const override { return ToolMode::Distance; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void reset();
    void reportMeasurement(const Point2D& p1, const Point2D& p2);

    bool m_hasFirst = false;
    Point2D m_firstPoint;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
