#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Linear dimension tool — measures and displays distance between two points
/// Creates dimension annotation as LineEntity (extension lines + dimension line) + TextEntity
class DimLinearTool : public Tool {
public:
    std::string name() const override { return "선형 치수"; }
    ToolMode mode() const override { return ToolMode::Dim; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void createDimension(const Point2D& p1, const Point2D& p2, const Point2D& dimLinePos);

    int m_clickCount = 0;
    Point2D m_point1;
    Point2D m_point2;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
