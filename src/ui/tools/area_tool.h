#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Area tool — click multiple vertices, right-click to finish; reports area via Shoelace formula
class AreaTool : public Tool {
public:
    std::string name() const override { return "면적계산"; }
    ToolMode mode() const override { return ToolMode::AreaCalc; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void finishArea();
    void rebuildPreview(const Point2D& cursor);

    /// Shoelace formula — returns signed area (positive CCW)
    static double shoelaceArea(const std::vector<Point2D>& pts);

    std::vector<Point2D> m_vertices;
    Point2D m_currentPos;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
