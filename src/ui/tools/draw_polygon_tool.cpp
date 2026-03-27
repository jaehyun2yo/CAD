#include "draw_polygon_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <cmath>
#include <QInputDialog>
#include <QWidget>

namespace yjcad {

static constexpr double POLY_PI = 3.14159265358979323846;

// ── DrawPolygonTool ──

bool DrawPolygonTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_hasCenter) {
        m_center = worldPos;
        m_hasCenter = true;
        return true;
    }

    // Second click — create regular polygon
    double radius = m_center.distanceTo(worldPos);
    if (radius < 1e-9) return true;

    std::vector<Point2D> verts = buildPolygonVertices(m_center, worldPos);

    EntityId id = m_doc->nextId();
    auto poly = std::make_unique<PolylineEntity>(id, verts, true);
    poly->setLayerName(m_doc->layers().currentLayer());
    auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(poly));
    m_doc->history().execute(std::move(cmd));

    m_hasCenter = false;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool DrawPolygonTool::mouseMove(const Point2D& worldPos) {
    m_currentPos = worldPos;
    m_preview.clear();
    if (m_hasCenter) {
        buildPolygonPreview(m_center, worldPos);
    }
    return m_hasCenter;
}

bool DrawPolygonTool::keyPress(int key) {
    if (key == Qt::Key_N) {
        // Ask user for number of sides
        QWidget* parent = m_canvas ? dynamic_cast<QWidget*>(m_canvas) : nullptr;
        bool ok = false;
        int sides = QInputDialog::getInt(parent,
            "다각형 변 수 입력", "변 수 (3~360):",
            m_sides, 3, 360, 1, &ok);
        if (ok && sides >= 3) {
            m_sides = sides;
            // Rebuild preview with new sides
            if (m_hasCenter) {
                m_preview.clear();
                buildPolygonPreview(m_center, m_currentPos);
            }
        }
        return true;
    }
    return false;
}

void DrawPolygonTool::cancel() {
    m_hasCenter = false;
    m_preview.clear();
}

std::vector<Point2D> DrawPolygonTool::buildPolygonVertices(
        const Point2D& center, const Point2D& radiusPt) const {
    double radius = center.distanceTo(radiusPt);
    double startAngle = std::atan2(radiusPt.y - center.y, radiusPt.x - center.x);

    std::vector<Point2D> verts;
    verts.reserve(m_sides);
    for (int i = 0; i < m_sides; ++i) {
        double angle = startAngle + 2.0 * POLY_PI * i / m_sides;
        verts.push_back({center.x + radius * std::cos(angle),
                         center.y + radius * std::sin(angle)});
    }
    return verts;
}

void DrawPolygonTool::buildPolygonPreview(const Point2D& center,
                                           const Point2D& radiusPt) {
    double radius = center.distanceTo(radiusPt);
    if (radius < 1e-9) return;

    std::vector<Point2D> verts = buildPolygonVertices(center, radiusPt);
    for (int i = 0; i < m_sides; ++i) {
        int next = (i + 1) % m_sides;
        m_preview.push_back({verts[i], verts[next]});
    }
}

} // namespace yjcad
