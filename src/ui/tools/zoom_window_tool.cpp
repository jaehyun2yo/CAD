#include "zoom_window_tool.h"
#include "ui/cad_canvas.h"
#include "core/renderer/camera.h"

namespace yjcad {

// ── ZoomWindowTool ──

bool ZoomWindowTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_hasFirst) {
        m_firstCorner = worldPos;
        m_hasFirst = true;
        return true;
    }

    // Second click — zoom to the selected box
    Point2D c1 = m_firstCorner;
    Point2D c2 = worldPos;

    BoundingBox box;
    box.min = Point2D{std::min(c1.x, c2.x), std::min(c1.y, c2.y)};
    box.max = Point2D{std::max(c1.x, c2.x), std::max(c1.y, c2.y)};

    if (m_canvas && box.isValid()) {
        m_canvas->camera().zoomToFit(box);
        m_canvas->refreshEntities();
    }

    // Auto-cancel back to select after zoom
    cancel();
    return true;
}

bool ZoomWindowTool::mouseMove(const Point2D& worldPos) {
    if (m_hasFirst) {
        rebuildPreview(worldPos);
    }
    return m_hasFirst;
}

void ZoomWindowTool::cancel() {
    m_hasFirst = false;
    m_preview.clear();
}

void ZoomWindowTool::rebuildPreview(const Point2D& corner2) {
    m_preview.clear();
    Point2D c1 = m_firstCorner;
    Point2D c2 = corner2;

    // Rectangle: four edges
    Point2D tl{std::min(c1.x, c2.x), std::max(c1.y, c2.y)};
    Point2D tr{std::max(c1.x, c2.x), std::max(c1.y, c2.y)};
    Point2D br{std::max(c1.x, c2.x), std::min(c1.y, c2.y)};
    Point2D bl{std::min(c1.x, c2.x), std::min(c1.y, c2.y)};

    m_preview.push_back({tl, tr});
    m_preview.push_back({tr, br});
    m_preview.push_back({br, bl});
    m_preview.push_back({bl, tl});
}

} // namespace yjcad
