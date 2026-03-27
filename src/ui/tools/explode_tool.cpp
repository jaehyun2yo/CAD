#include "explode_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"

namespace yjcad {

// ── ExplodeTool ──

bool ExplodeTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    (void)worldPos;
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_doc || m_doc->selection().empty()) return false;

    executeExplode();
    return true;
}

bool ExplodeTool::mouseMove(const Point2D& worldPos) {
    (void)worldPos;
    return false;
}

void ExplodeTool::cancel() {
    m_preview.clear();
}

void ExplodeTool::executeExplode() {
    if (!m_doc) return;

    auto selectedIds = std::vector<EntityId>(
        m_doc->selection().selectedIds().begin(),
        m_doc->selection().selectedIds().end());

    std::vector<EntityId> polyIds;
    for (EntityId id : selectedIds) {
        Entity* ent = m_doc->entity(id);
        if (ent && ent->type() == EntityType::Polyline) {
            polyIds.push_back(id);
        }
    }

    if (polyIds.empty()) return;

    for (EntityId polyId : polyIds) {
        auto* poly = static_cast<PolylineEntity*>(m_doc->entity(polyId));
        if (!poly) continue;

        const auto& verts = poly->vertices();
        const bool closed = poly->isClosed();
        const std::string layerName = poly->layerName();
        const int color = poly->color();
        const LineClassification cls = poly->classification();

        // Collect segments
        std::vector<std::pair<Point2D, Point2D>> segments;
        for (size_t i = 0; i + 1 < verts.size(); ++i) {
            segments.emplace_back(verts[i], verts[i + 1]);
        }
        // Closed: last vertex → first vertex
        if (closed && verts.size() >= 2) {
            segments.emplace_back(verts.back(), verts.front());
        }

        // Remove the polyline first
        auto removeCmd = std::make_unique<RemoveEntityCommand>(*m_doc, polyId);
        m_doc->history().execute(std::move(removeCmd));

        // Add individual line entities
        for (const auto& seg : segments) {
            EntityId newId = m_doc->nextId();
            auto line = std::make_unique<LineEntity>(newId, seg.first, seg.second);
            line->setLayerName(layerName);
            line->setColor(color);
            line->setClassification(cls);

            auto addCmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(line));
            m_doc->history().execute(std::move(addCmd));
        }
    }

    m_doc->selection().clear();
    if (m_canvas) m_canvas->refreshEntities();
}

} // namespace yjcad
