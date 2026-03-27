#include "break_tool.h"
#include "core/document/document.h"
#include "core/geometry/operations.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

bool BreakTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_lineSelected) {
        // Phase 1: pick the closest line entity
        EntityId closestId = 0;
        double closestDist = 1e18;

        for (EntityId id : m_doc->entityIds()) {
            auto* entity = m_doc->entity(id);
            if (!entity || !entity->isVisible()) continue;
            if (entity->type() != EntityType::Line) continue;

            auto* line = static_cast<LineEntity*>(entity);
            double d = GeometryOps::pointToSegmentDistance(
                worldPos, line->start(), line->end());
            if (d < closestDist) {
                closestDist = d;
                closestId = id;
            }
        }

        if (closestId == 0) return false;

        m_targetId = closestId;
        m_lineSelected = true;

        // Preview: highlight the selected line
        auto* line = static_cast<LineEntity*>(m_doc->entity(m_targetId));
        if (line) {
            m_preview.clear();
            m_preview.push_back({line->start(), line->end()});
        }
        return true;
    }

    // Phase 2: split the selected line at the clicked point
    auto* entity = m_doc->entity(m_targetId);
    if (!entity || entity->type() != EntityType::Line) {
        cancel();
        return false;
    }

    auto* line = static_cast<LineEntity*>(entity);

    // Project the click point onto the line segment
    double dx = line->end().x - line->start().x;
    double dy = line->end().y - line->start().y;
    double lenSq = dx * dx + dy * dy;

    Point2D splitPt = worldPos;
    if (lenSq > 1e-18) {
        double t = ((worldPos.x - line->start().x) * dx +
                    (worldPos.y - line->start().y) * dy) / lenSq;
        t = std::clamp(t, 0.0, 1.0);
        splitPt = {line->start().x + t * dx, line->start().y + t * dy};
    }

    // Use splitLineAt to get the two halves
    EntityId startId = m_doc->nextId();
    auto segments = GeometryOps::splitLineAt(*line, splitPt, startId);

    if (segments.empty()) { cancel(); return false; }

    // Copy layer/color/classification from original
    std::string layerName = line->layerName();
    int color             = line->color();
    auto classif          = line->classification();

    // Remove original
    auto rmCmd = std::make_unique<RemoveEntityCommand>(*m_doc, m_targetId);
    m_doc->history().execute(std::move(rmCmd));

    // Add each segment
    for (auto& seg : segments) {
        seg->setLayerName(layerName);
        seg->setColor(color);
        seg->setClassification(classif);
        auto addCmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(seg));
        m_doc->history().execute(std::move(addCmd));
    }

    cancel();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool BreakTool::mouseMove(const Point2D& worldPos) {
    if (!m_lineSelected) return false;

    auto* entity = m_doc->entity(m_targetId);
    if (!entity || entity->type() != EntityType::Line) return false;

    auto* line = static_cast<LineEntity*>(entity);

    // Project cursor onto line and show split preview
    double dx = line->end().x - line->start().x;
    double dy = line->end().y - line->start().y;
    double lenSq = dx * dx + dy * dy;

    Point2D splitPt = worldPos;
    if (lenSq > 1e-18) {
        double t = ((worldPos.x - line->start().x) * dx +
                    (worldPos.y - line->start().y) * dy) / lenSq;
        t = std::clamp(t, 0.0, 1.0);
        splitPt = {line->start().x + t * dx, line->start().y + t * dy};
    }

    m_preview.clear();
    m_preview.push_back({line->start(), splitPt});
    m_preview.push_back({splitPt, line->end()});
    return true;
}

void BreakTool::cancel() {
    m_lineSelected = false;
    m_targetId = 0;
    m_preview.clear();
}

} // namespace yjcad
