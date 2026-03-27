#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Interactive regular polygon drawing tool
/// 2-click: center → radius point → regular N-gon PolylineEntity (closed)
/// Press N key to change number of sides (default 6)
class DrawPolygonTool : public Tool {
public:
    std::string name() const override { return "다각형"; }
    ToolMode mode() const override { return ToolMode::DrawPolygon; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    bool keyPress(int key) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

    int sides() const { return m_sides; }

private:
    int m_sides = 6;
    bool m_hasCenter = false;
    Point2D m_center;
    Point2D m_currentPos;
    std::vector<std::pair<Point2D, Point2D>> m_preview;

    void buildPolygonPreview(const Point2D& center, const Point2D& radiusPt);
    std::vector<Point2D> buildPolygonVertices(const Point2D& center,
                                               const Point2D& radiusPt) const;
};

} // namespace yjcad
