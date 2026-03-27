#include "draw_xline_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

// ── DrawXLineTool ──

bool DrawXLineTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_hasOrigin) {
        m_origin = worldPos;
        m_hasOrigin = true;
        return true;
    }

    // Second click — create construction line
    double dist = m_origin.distanceTo(worldPos);
    if (dist < 1e-9) return true;

    auto [lineStart, lineEnd] = buildXLine(m_origin, worldPos);

    EntityId id = m_doc->nextId();
    auto line = std::make_unique<LineEntity>(id, lineStart, lineEnd);
    line->setLayerName(GUIDE_LAYER);
    line->setClassification(LineClassification::Guide);
    auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(line));
    m_doc->history().execute(std::move(cmd));

    // Continue from origin — allow drawing more parallel lines
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool DrawXLineTool::mouseMove(const Point2D& worldPos) {
    m_currentPos = worldPos;
    m_preview.clear();
    if (m_hasOrigin) {
        double dist = m_origin.distanceTo(worldPos);
        if (dist > 1e-9) {
            auto [p1, p2] = buildXLine(m_origin, worldPos);
            m_preview.push_back({p1, p2});
        }
    }
    return m_hasOrigin;
}

void DrawXLineTool::cancel() {
    m_hasOrigin = false;
    m_preview.clear();
}

std::pair<Point2D, Point2D> DrawXLineTool::buildXLine(
        const Point2D& origin, const Point2D& directionPt) const {
    double dx = directionPt.x - origin.x;
    double dy = directionPt.y - origin.y;
    double len = std::sqrt(dx * dx + dy * dy);

    // Normalize direction
    double ux = dx / len;
    double uy = dy / len;

    Point2D p1{origin.x - ux * EXTEND_LENGTH, origin.y - uy * EXTEND_LENGTH};
    Point2D p2{origin.x + ux * EXTEND_LENGTH, origin.y + uy * EXTEND_LENGTH};
    return {p1, p2};
}

} // namespace yjcad
