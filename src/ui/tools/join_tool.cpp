#include "join_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <unordered_set>
#include <unordered_map>

namespace yjcad {

// ── JoinTool ──

bool JoinTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    (void)worldPos;
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_doc || m_doc->selection().empty()) return false;

    executeJoin();
    return true;
}

bool JoinTool::mouseMove(const Point2D& worldPos) {
    (void)worldPos;
    return false;
}

void JoinTool::cancel() {
    m_preview.clear();
}

void JoinTool::executeJoin() {
    if (!m_doc) return;

    // Collect selected LineEntity ids
    auto selectedIds = std::vector<EntityId>(
        m_doc->selection().selectedIds().begin(),
        m_doc->selection().selectedIds().end());

    std::vector<EntityId> lineIds;
    for (EntityId id : selectedIds) {
        Entity* ent = m_doc->entity(id);
        if (ent && ent->type() == EntityType::Line) {
            lineIds.push_back(id);
        }
    }

    if (lineIds.size() < 2) return;

    // Build adjacency: for each line store {start, end}
    struct Seg {
        EntityId id;
        Point2D start;
        Point2D end;
    };

    std::vector<Seg> segs;
    segs.reserve(lineIds.size());
    for (EntityId id : lineIds) {
        auto* line = static_cast<LineEntity*>(m_doc->entity(id));
        segs.push_back({id, line->start(), line->end()});
    }

    // Greedy chain building: pick an unused segment, extend as far as possible
    std::vector<bool> used(segs.size(), false);
    std::vector<std::vector<size_t>> chains; // indices into segs

    for (size_t i = 0; i < segs.size(); ++i) {
        if (used[i]) continue;

        std::vector<size_t> chain;
        chain.push_back(i);
        used[i] = true;

        // Extend forward from chain.back() end
        bool extended = true;
        while (extended) {
            extended = false;
            Point2D tip = segs[chain.back()].end;
            for (size_t j = 0; j < segs.size(); ++j) {
                if (used[j]) continue;
                if (tip.distanceTo(segs[j].start) < kEpsilon) {
                    chain.push_back(j);
                    used[j] = true;
                    extended = true;
                    break;
                }
                if (tip.distanceTo(segs[j].end) < kEpsilon) {
                    // Reverse this segment direction by swapping start/end
                    std::swap(segs[j].start, segs[j].end);
                    chain.push_back(j);
                    used[j] = true;
                    extended = true;
                    break;
                }
            }
        }

        // Extend backward from chain.front() start
        extended = true;
        while (extended) {
            extended = false;
            Point2D tail = segs[chain.front()].start;
            for (size_t j = 0; j < segs.size(); ++j) {
                if (used[j]) continue;
                if (tail.distanceTo(segs[j].end) < kEpsilon) {
                    chain.insert(chain.begin(), j);
                    used[j] = true;
                    extended = true;
                    break;
                }
                if (tail.distanceTo(segs[j].start) < kEpsilon) {
                    std::swap(segs[j].start, segs[j].end);
                    chain.insert(chain.begin(), j);
                    used[j] = true;
                    extended = true;
                    break;
                }
            }
        }

        if (chain.size() >= 2) {
            chains.push_back(std::move(chain));
        }
    }

    if (chains.empty()) return;

    // For each chain: remove originals, add polyline
    for (const auto& chain : chains) {
        // Build vertex list
        std::vector<Point2D> vertices;
        vertices.push_back(segs[chain.front()].start);
        for (size_t idx : chain) {
            vertices.push_back(segs[idx].end);
        }

        // Check closed
        bool closed = false;
        if (vertices.size() >= 3 &&
            vertices.front().distanceTo(vertices.back()) < kEpsilon) {
            vertices.pop_back();
            closed = true;
        }

        // Remove source lines
        for (size_t idx : chain) {
            auto cmd = std::make_unique<RemoveEntityCommand>(*m_doc, segs[idx].id);
            m_doc->history().execute(std::move(cmd));
        }

        // Add polyline
        EntityId newId = m_doc->nextId();
        auto poly = std::make_unique<PolylineEntity>(newId, vertices, closed);
        // Inherit layer from first source line
        Entity* firstEnt = m_doc->entity(segs[chain.front()].id);
        if (!firstEnt) {
            // Already removed — use current layer
            poly->setLayerName(m_doc->layers().currentLayer());
        } else {
            poly->setLayerName(firstEnt->layerName());
        }
        poly->setLayerName(m_doc->layers().currentLayer());

        auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(poly));
        m_doc->history().execute(std::move(cmd));
    }

    m_doc->selection().clear();
    if (m_canvas) m_canvas->refreshEntities();
}

} // namespace yjcad
