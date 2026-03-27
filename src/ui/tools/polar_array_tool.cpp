#include "polar_array_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

static constexpr double kPi = 3.14159265358979323846;

bool PolarArrayTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (m_doc->selection().empty()) return false;

    if (!m_hasCenter) {
        m_center = worldPos;
        m_hasCenter = true;
        return true;
    }

    // Second click: execute polar array
    executePolarArray(worldPos);
    cancel();
    return true;
}

bool PolarArrayTool::mouseMove(const Point2D& worldPos) {
    if (!m_hasCenter) return false;
    updatePreview(worldPos);
    return true;
}

bool PolarArrayTool::keyPress(int key) {
    if (key == Qt::Key_N) {
        // Uppercase N = increase count
        m_count = std::min(m_count + 1, 360);
        return true;
    }
    if (key == Qt::Key_M) {
        // M to decrease (use a distinct key to avoid conflicts)
        m_count = std::max(m_count - 1, 2);
        return true;
    }
    return false;
}

void PolarArrayTool::executePolarArray(const Point2D& refPoint) {
    if (m_count < 2) return;

    auto selectedIds = std::vector<EntityId>(
        m_doc->selection().selectedIds().begin(),
        m_doc->selection().selectedIds().end());

    double stepDeg = 360.0 / m_count;

    // Start at 1 — index 0 is the original
    for (int i = 1; i < m_count; ++i) {
        double angleDeg = i * stepDeg;

        for (EntityId srcId : selectedIds) {
            auto* src = m_doc->entity(srcId);
            if (!src) continue;

            auto copy = src->clone();
            copy->rotate(m_center, angleDeg);
            auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(copy));
            m_doc->history().execute(std::move(cmd));
        }
    }

    if (m_canvas) m_canvas->refreshEntities();
    (void)refPoint;
}

void PolarArrayTool::updatePreview(const Point2D& currentPos) {
    m_preview.clear();

    // Draw reference radius line
    m_preview.push_back({m_center, currentPos});

    // Draw arc indicator segments (approximated as chords)
    double radius = m_center.distanceTo(currentPos);
    if (radius < 1e-9) return;

    double stepDeg = 360.0 / m_count;
    for (int i = 0; i < m_count; ++i) {
        double a1 = i * stepDeg * kPi / 180.0;
        double a2 = (i + 1) * stepDeg * kPi / 180.0;
        Point2D p1{m_center.x + radius * std::cos(a1),
                   m_center.y + radius * std::sin(a1)};
        Point2D p2{m_center.x + radius * std::cos(a2),
                   m_center.y + radius * std::sin(a2)};
        m_preview.push_back({p1, p2});
    }
}

void PolarArrayTool::cancel() {
    m_hasCenter = false;
    m_preview.clear();
}

} // namespace yjcad
