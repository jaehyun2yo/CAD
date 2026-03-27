#include "scale_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"

namespace yjcad {

bool ScaleTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;
    if (m_doc->selection().empty()) return false;

    if (m_clickCount == 0) {
        // First click: base point
        m_basePoint = worldPos;
        m_clickCount = 1;
        return true;
    }

    if (m_clickCount == 1) {
        // Second click: reference distance
        m_refPoint = worldPos;
        m_refDist = m_basePoint.distanceTo(worldPos);
        if (m_refDist < 1e-9) return true; // ignore zero distance
        m_clickCount = 2;
        return true;
    }

    // Third click: target distance → compute scale factor
    double targetDist = m_basePoint.distanceTo(worldPos);
    double factor = targetDist / m_refDist;
    if (factor < 1e-9) return true;

    auto ids = std::vector<EntityId>(
        m_doc->selection().selectedIds().begin(),
        m_doc->selection().selectedIds().end());

    auto cmd = std::make_unique<ScaleCommand>(*m_doc, ids, m_basePoint, factor);
    m_doc->history().execute(std::move(cmd));

    m_clickCount = 0;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool ScaleTool::mouseMove(const Point2D& worldPos) {
    m_preview.clear();
    if (m_clickCount >= 1) {
        m_preview.push_back({m_basePoint, worldPos});
    }
    if (m_clickCount == 2) {
        m_preview.push_back({m_basePoint, m_refPoint});
    }
    return m_clickCount > 0;
}

void ScaleTool::cancel() {
    m_clickCount = 0;
    m_preview.clear();
}

} // namespace yjcad
