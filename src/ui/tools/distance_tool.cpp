#include "distance_tool.h"
#include <cmath>
#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(lcDistance, "yjcad.tool.distance")

namespace yjcad {

// ── DistanceTool ──

bool DistanceTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_hasFirst) {
        m_firstPoint = worldPos;
        m_hasFirst = true;
        return true;
    }

    // Second click — compute and report
    reportMeasurement(m_firstPoint, worldPos);
    reset();
    return true;
}

bool DistanceTool::mouseMove(const Point2D& worldPos) {
    m_preview.clear();
    if (m_hasFirst) {
        m_preview.push_back({m_firstPoint, worldPos});
    }
    return m_hasFirst;
}

void DistanceTool::cancel() {
    reset();
}

void DistanceTool::reset() {
    m_hasFirst = false;
    m_preview.clear();
}

void DistanceTool::reportMeasurement(const Point2D& p1, const Point2D& p2) {
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    double angleDeg = std::atan2(dy, dx) * 180.0 / M_PI;

    qCInfo(lcDistance,
           "거리: %.4f mm  각도: %.2f°  DX: %.4f  DY: %.4f",
           dist, angleDeg, dx, dy);
}

} // namespace yjcad
