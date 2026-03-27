#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Interactive rectangle drawing tool
/// Click first corner → rubber-band preview → click opposite corner → create PolylineEntity
class DrawRectTool : public Tool {
public:
    std::string name() const override { return "사각형"; }
    ToolMode mode() const override { return ToolMode::DrawRect; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    bool m_hasFirst = false;
    Point2D m_firstPoint;
    Point2D m_currentPos;
    std::vector<std::pair<Point2D, Point2D>> m_preview;

    void buildRectPreview(const Point2D& p1, const Point2D& p2);
};

} // namespace yjcad
