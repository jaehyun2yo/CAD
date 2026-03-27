#include "draw_arc_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;

bool DrawArcTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (m_clickCount == 0) {
        // First click: center
        m_center = worldPos;
        m_clickCount = 1;
        return true;
    }

    if (m_clickCount == 1) {
        // Second click: start point (defines radius + start angle)
        m_startPoint = worldPos;
        m_radius = m_center.distanceTo(worldPos);
        if (m_radius < 1e-9) return true; // ignore zero-radius
        m_startAngleDeg = std::atan2(worldPos.y - m_center.y,
                                      worldPos.x - m_center.x) * 180.0 / PI;
        m_clickCount = 2;
        return true;
    }

    // Third click: end point (defines end angle)
    double endAngleDeg = std::atan2(worldPos.y - m_center.y,
                                     worldPos.x - m_center.x) * 180.0 / PI;

    // Normalize angles to [0, 360)
    double startA = m_startAngleDeg;
    double endA = endAngleDeg;
    if (startA < 0) startA += 360.0;
    if (endA < 0) endA += 360.0;

    EntityId id = m_doc->nextId();
    auto arc = std::make_unique<ArcEntity>(id, m_center, m_radius, startA, endA);
    arc->setLayerName(m_doc->layers().currentLayer());
    auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(arc));
    m_doc->history().execute(std::move(cmd));

    m_clickCount = 0;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool DrawArcTool::mouseMove(const Point2D& worldPos) {
    m_preview.clear();

    if (m_clickCount == 1) {
        // Show radius line from center to cursor
        m_preview.push_back({m_center, worldPos});
        // Show full circle preview
        double r = m_center.distanceTo(worldPos);
        if (r > 1e-9) buildArcPreview(m_center, r, 0, 360);
    } else if (m_clickCount == 2) {
        // Show arc preview from start angle to cursor angle
        double endAngleDeg = std::atan2(worldPos.y - m_center.y,
                                         worldPos.x - m_center.x) * 180.0 / PI;
        double startA = m_startAngleDeg;
        double endA = endAngleDeg;
        if (startA < 0) startA += 360.0;
        if (endA < 0) endA += 360.0;

        buildArcPreview(m_center, m_radius, startA, endA);
        // Show radius lines
        m_preview.push_back({m_center, m_startPoint});
        m_preview.push_back({m_center, worldPos});
    }

    return m_clickCount > 0;
}

void DrawArcTool::cancel() {
    m_clickCount = 0;
    m_preview.clear();
}

void DrawArcTool::buildArcPreview(const Point2D& center, double radius,
                                   double startDeg, double endDeg) {
    double startRad = startDeg * PI / 180.0;
    double endRad = endDeg * PI / 180.0;
    if (endRad <= startRad) endRad += 2.0 * PI;
    double span = endRad - startRad;

    int segs = std::max(12, static_cast<int>(span / (2.0 * PI) * 48));
    for (int i = 0; i < segs; ++i) {
        double a1 = startRad + span * i / segs;
        double a2 = startRad + span * (i + 1) / segs;
        Point2D p1{center.x + radius * std::cos(a1), center.y + radius * std::sin(a1)};
        Point2D p2{center.x + radius * std::cos(a2), center.y + radius * std::sin(a2)};
        m_preview.push_back({p1, p2});
    }
}

} // namespace yjcad
