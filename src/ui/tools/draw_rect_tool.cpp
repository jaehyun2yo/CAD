#include "draw_rect_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"

namespace yjcad {

// ── DrawRectTool ──

bool DrawRectTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_hasFirst) {
        m_firstPoint = worldPos;
        m_hasFirst = true;
        return true;
    }

    // Second click — create closed polyline (4 corners)
    const Point2D& p1 = m_firstPoint;
    const Point2D& p2 = worldPos;

    std::vector<Point2D> verts = {
        p1,
        Point2D{p2.x, p1.y},
        p2,
        Point2D{p1.x, p2.y}
    };

    EntityId id = m_doc->nextId();
    auto poly = std::make_unique<PolylineEntity>(id, verts, true);
    poly->setLayerName(m_doc->layers().currentLayer());
    auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(poly));
    m_doc->history().execute(std::move(cmd));

    m_hasFirst = false;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool DrawRectTool::mouseMove(const Point2D& worldPos) {
    m_currentPos = worldPos;
    m_preview.clear();
    if (m_hasFirst) {
        buildRectPreview(m_firstPoint, worldPos);
    }
    return m_hasFirst;
}

void DrawRectTool::cancel() {
    m_hasFirst = false;
    m_preview.clear();
}

void DrawRectTool::buildRectPreview(const Point2D& p1, const Point2D& p2) {
    Point2D tl{p1.x, p1.y};
    Point2D tr{p2.x, p1.y};
    Point2D br{p2.x, p2.y};
    Point2D bl{p1.x, p2.y};

    m_preview.push_back({tl, tr});
    m_preview.push_back({tr, br});
    m_preview.push_back({br, bl});
    m_preview.push_back({bl, tl});
}

} // namespace yjcad
