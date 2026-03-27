#include "draw_point_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"

namespace yjcad {

// ── DrawPointTool ──

bool DrawPointTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) return true;
    if (button != Qt::LeftButton) return false;

    const std::string& layer = m_doc->layers().currentLayer();

    // Horizontal line: (x-1, y) → (x+1, y)
    {
        Point2D left {worldPos.x - MARKER_HALF, worldPos.y};
        Point2D right{worldPos.x + MARKER_HALF, worldPos.y};
        EntityId id = m_doc->nextId();
        auto line = std::make_unique<LineEntity>(id, left, right);
        line->setLayerName(layer);
        auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(line));
        m_doc->history().execute(std::move(cmd));
    }

    // Vertical line: (x, y-1) → (x, y+1)
    {
        Point2D bottom{worldPos.x, worldPos.y - MARKER_HALF};
        Point2D top   {worldPos.x, worldPos.y + MARKER_HALF};
        EntityId id = m_doc->nextId();
        auto line = std::make_unique<LineEntity>(id, bottom, top);
        line->setLayerName(layer);
        auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(line));
        m_doc->history().execute(std::move(cmd));
    }

    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

} // namespace yjcad
