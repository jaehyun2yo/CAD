#include "copy_rotate_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

// ── CopyTool ──

bool CopyTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;
    if (m_doc->selection().empty()) return false;

    if (!m_hasBase) {
        m_basePoint = worldPos;
        m_hasBase = true;
        return true;
    }

    // Copy: clone selected entities and move clones
    Point2D delta{worldPos.x - m_basePoint.x, worldPos.y - m_basePoint.y};
    for (EntityId id : m_doc->selection().selectedIds()) {
        const Entity* orig = m_doc->entity(id);
        if (!orig) continue;
        auto clone = orig->clone();
        // Assign new ID
        EntityId newId = m_doc->nextId();
        // We need to create a new entity with new ID — clone keeps old ID
        // Workaround: translate the clone
        clone->translate(delta);
        // Add via command for undo support
        auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(clone));
        m_doc->history().execute(std::move(cmd));
    }

    m_hasBase = false;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool CopyTool::mouseMove(const Point2D& worldPos) {
    m_preview.clear();
    if (m_hasBase) {
        m_preview.push_back({m_basePoint, worldPos});
    }
    return m_hasBase;
}

void CopyTool::cancel() {
    m_hasBase = false;
    m_preview.clear();
}

// ── RotateTool ──

bool RotateTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;
    if (m_doc->selection().empty()) return false;

    if (m_clickCount == 0) {
        // First click: center of rotation
        m_center = worldPos;
        m_clickCount = 1;
        return true;
    }

    if (m_clickCount == 1) {
        // Second click: reference angle
        m_refPoint = worldPos;
        m_clickCount = 2;
        return true;
    }

    // Third click: target angle → rotate
    double refAngle = std::atan2(m_refPoint.y - m_center.y, m_refPoint.x - m_center.x);
    double targetAngle = std::atan2(worldPos.y - m_center.y, worldPos.x - m_center.x);
    double angleDeg = (targetAngle - refAngle) * 180.0 / 3.14159265358979323846;

    for (EntityId id : m_doc->selection().selectedIds()) {
        if (auto* e = m_doc->entity(id)) {
            e->rotate(m_center, angleDeg);
        }
    }
    m_doc->setModified(true);

    m_clickCount = 0;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool RotateTool::mouseMove(const Point2D& worldPos) {
    m_preview.clear();
    if (m_clickCount >= 1) {
        m_preview.push_back({m_center, worldPos});
    }
    if (m_clickCount == 2) {
        m_preview.push_back({m_center, m_refPoint});
    }
    return m_clickCount > 0;
}

void RotateTool::cancel() {
    m_clickCount = 0;
    m_preview.clear();
}

} // namespace yjcad
