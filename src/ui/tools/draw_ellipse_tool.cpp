#include "draw_ellipse_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;

// ── DrawEllipseTool ──

bool DrawEllipseTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (m_step == 0) {
        // Step 1: set center
        m_center = worldPos;
        m_step = 1;
        return true;
    }

    if (m_step == 1) {
        // Step 2: set X-axis endpoint
        double rx = m_center.distanceTo(worldPos);
        if (rx < 1e-9) return true;
        m_xAxisPt = worldPos;
        m_step = 2;
        return true;
    }

    // Step 3: set Y-radius — create ellipse as polyline
    double rx = m_center.distanceTo(m_xAxisPt);
    double ry = m_center.distanceTo(worldPos);
    if (rx < 1e-9 || ry < 1e-9) return true;

    // Rotation angle of major axis
    double axisAngle = std::atan2(m_xAxisPt.y - m_center.y,
                                   m_xAxisPt.x - m_center.x);
    double angleDeg = axisAngle * 180.0 / PI;

    // Build vertices
    std::vector<Point2D> verts;
    verts.reserve(SEGS);
    double cosA = std::cos(axisAngle);
    double sinA = std::sin(axisAngle);

    for (int i = 0; i < SEGS; ++i) {
        double t = 2.0 * PI * i / SEGS;
        double lx = rx * std::cos(t);
        double ly = ry * std::sin(t);
        // Rotate by major-axis angle
        double wx = m_center.x + lx * cosA - ly * sinA;
        double wy = m_center.y + lx * sinA + ly * cosA;
        verts.push_back({wx, wy});
    }

    EntityId id = m_doc->nextId();
    auto poly = std::make_unique<PolylineEntity>(id, verts, true);
    poly->setLayerName(m_doc->layers().currentLayer());
    auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(poly));
    m_doc->history().execute(std::move(cmd));

    cancel();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool DrawEllipseTool::mouseMove(const Point2D& worldPos) {
    m_currentPos = worldPos;
    m_preview.clear();

    if (m_step == 1) {
        // Preview: line from center to cursor (X-axis indicator)
        m_preview.push_back({m_center, worldPos});
    } else if (m_step == 2) {
        double rx = m_center.distanceTo(m_xAxisPt);
        double ry = m_center.distanceTo(worldPos);
        double axisAngle = std::atan2(m_xAxisPt.y - m_center.y,
                                       m_xAxisPt.x - m_center.x);
        if (rx > 1e-9 && ry > 1e-9) {
            buildEllipsePreview(m_center, rx, ry, axisAngle * 180.0 / PI);
        }
    }

    return m_step > 0;
}

void DrawEllipseTool::cancel() {
    m_step = 0;
    m_preview.clear();
}

void DrawEllipseTool::buildEllipsePreview(const Point2D& center,
                                           double rx, double ry,
                                           double angleDeg) {
    double axisAngle = angleDeg * PI / 180.0;
    double cosA = std::cos(axisAngle);
    double sinA = std::sin(axisAngle);

    for (int i = 0; i < SEGS; ++i) {
        double t1 = 2.0 * PI * i / SEGS;
        double t2 = 2.0 * PI * (i + 1) / SEGS;

        auto toWorld = [&](double t) -> Point2D {
            double lx = rx * std::cos(t);
            double ly = ry * std::sin(t);
            return {center.x + lx * cosA - ly * sinA,
                    center.y + lx * sinA + ly * cosA};
        };

        m_preview.push_back({toWorld(t1), toWorld(t2)});
    }
}

} // namespace yjcad
