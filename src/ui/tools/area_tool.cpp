#include "area_tool.h"
#include <cmath>
#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(lcArea, "yjcad.tool.area")

namespace yjcad {

// ── AreaTool ──

bool AreaTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) {
        if (m_vertices.size() >= 3) {
            finishArea();
        } else {
            cancel();
        }
        return true;
    }
    if (button != Qt::LeftButton) return false;

    m_vertices.push_back(worldPos);
    m_currentPos = worldPos;
    rebuildPreview(worldPos);
    return true;
}

bool AreaTool::mouseMove(const Point2D& worldPos) {
    m_currentPos = worldPos;
    rebuildPreview(worldPos);
    return !m_vertices.empty();
}

void AreaTool::cancel() {
    m_vertices.clear();
    m_preview.clear();
}

void AreaTool::finishArea() {
    if (m_vertices.size() < 3) return;

    double area = std::abs(shoelaceArea(m_vertices));
    // Convert mm² to cm² for display (1 cm² = 100 mm²)
    double areaCm2 = area / 100.0;

    qCInfo(lcArea,
           "면적: %.4f mm²  (%.4f cm²)  꼭짓점 수: %zu",
           area, areaCm2, m_vertices.size());

    cancel();
}

void AreaTool::rebuildPreview(const Point2D& cursor) {
    m_preview.clear();

    // Draw committed segments
    for (size_t i = 0; i + 1 < m_vertices.size(); ++i) {
        m_preview.push_back({m_vertices[i], m_vertices[i + 1]});
    }

    // Rubber-band: last vertex → cursor
    if (!m_vertices.empty()) {
        m_preview.push_back({m_vertices.back(), cursor});
        // Closing edge: cursor → first vertex (preview only)
        if (m_vertices.size() >= 2) {
            m_preview.push_back({cursor, m_vertices.front()});
        }
    }
}

double AreaTool::shoelaceArea(const std::vector<Point2D>& pts) {
    double sum = 0.0;
    size_t n = pts.size();
    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        sum += pts[i].x * pts[j].y;
        sum -= pts[j].x * pts[i].y;
    }
    return 0.5 * sum;
}

} // namespace yjcad
