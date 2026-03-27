#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Zoom Window tool — click two diagonal corners to zoom into a rectangular region
class ZoomWindowTool : public Tool {
public:
    std::string name() const override { return "줌 윈도"; }
    ToolMode mode() const override { return ToolMode::ZoomWindow; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void rebuildPreview(const Point2D& corner2);

    bool m_hasFirst = false;
    Point2D m_firstCorner;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
