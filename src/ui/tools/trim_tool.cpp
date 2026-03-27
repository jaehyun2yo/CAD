#include "trim_tool.h"
#include "core/document/document.h"
#include "core/geometry/operations.h"
#include "ui/cad_canvas.h"
#include <algorithm>
#include <cmath>

namespace yjcad {

bool TrimTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    // On first use, capture current selection as boundary entities
    if (!m_boundariesSet) {
        auto selected = m_doc->selection().selectedIds();
        m_boundaryIds.assign(selected.begin(), selected.end());
        if (m_boundaryIds.empty()) return false;
        m_boundariesSet = true;
        m_doc->selection().clear();
        return true;
    }

    // Click on a line to trim — find the closest line entity to the click point
    EntityId closestId = 0;
    double closestDist = 1e18;
    for (EntityId id : m_doc->entityIds()) {
        auto* entity = m_doc->entity(id);
        if (!entity || !entity->isVisible()) continue;
        if (entity->type() != EntityType::Line) continue;
        // Skip boundary entities
        bool isBoundary = false;
        for (EntityId bid : m_boundaryIds) {
            if (id == bid) { isBoundary = true; break; }
        }
        if (isBoundary) continue;

        auto* line = static_cast<LineEntity*>(entity);
        double d = GeometryOps::pointToSegmentDistance(worldPos, line->start(), line->end());
        if (d < closestDist) {
            closestDist = d;
            closestId = id;
        }
    }

    if (closestId == 0) return false;

    auto* line = static_cast<LineEntity*>(m_doc->entity(closestId));
    if (!line) return false;

    // Find intersections
    auto intersections = findIntersections(*line);
    if (intersections.empty()) return false; // no intersections

    // Sort by parameter t
    std::sort(intersections.begin(), intersections.end(),
              [](const auto& a, const auto& b) { return a.t < b.t; });

    // Determine which segment the click point is on
    // Compute t parameter for click point
    double dx = line->end().x - line->start().x;
    double dy = line->end().y - line->start().y;
    double lenSq = dx * dx + dy * dy;
    double clickT = 0.5;
    if (lenSq > 1e-18) {
        clickT = ((worldPos.x - line->start().x) * dx +
                  (worldPos.y - line->start().y) * dy) / lenSq;
        clickT = std::clamp(clickT, 0.0, 1.0);
    }

    // Find which segment to remove: the one containing clickT
    // Segments are: [0, t0], [t0, t1], ..., [tn, 1]
    std::vector<double> cuts;
    cuts.push_back(0.0);
    for (const auto& ix : intersections) {
        cuts.push_back(ix.t);
    }
    cuts.push_back(1.0);

    // Find segment containing clickT
    int removeIdx = -1;
    for (size_t i = 0; i + 1 < cuts.size(); ++i) {
        if (clickT >= cuts[i] - 1e-9 && clickT <= cuts[i + 1] + 1e-9) {
            removeIdx = static_cast<int>(i);
            break;
        }
    }
    if (removeIdx < 0) return false;

    // Remove original line, add remaining segments
    auto rmCmd = std::make_unique<RemoveEntityCommand>(*m_doc, closestId);
    m_doc->history().execute(std::move(rmCmd));

    for (size_t i = 0; i + 1 < cuts.size(); ++i) {
        if (static_cast<int>(i) == removeIdx) continue;
        double t1 = cuts[i], t2 = cuts[i + 1];
        if (t2 - t1 < 1e-9) continue;

        Point2D p1{line->start().x + t1 * dx, line->start().y + t1 * dy};
        Point2D p2{line->start().x + t2 * dx, line->start().y + t2 * dy};
        auto seg = std::make_unique<LineEntity>(m_doc->nextId(), p1, p2);
        seg->setLayerName(line->layerName());
        seg->setColor(line->color());
        seg->setClassification(line->classification());
        auto addCmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(seg));
        m_doc->history().execute(std::move(addCmd));
    }

    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool TrimTool::mouseMove(const Point2D& worldPos) {
    (void)worldPos;
    return false;
}

void TrimTool::cancel() {
    m_boundaryIds.clear();
    m_boundariesSet = false;
    m_preview.clear();
}

std::vector<TrimTool::IntersectionInfo> TrimTool::findIntersections(const LineEntity& line) {
    std::vector<IntersectionInfo> result;

    double dx = line.end().x - line.start().x;
    double dy = line.end().y - line.start().y;
    double lineLen = std::sqrt(dx * dx + dy * dy);
    if (lineLen < 1e-12) return result;

    for (EntityId bid : m_boundaryIds) {
        auto* boundary = m_doc->entity(bid);
        if (!boundary) continue;

        if (boundary->type() == EntityType::Line) {
            auto* bLine = static_cast<LineEntity*>(boundary);
            Point2D ix;
            if (GeometryOps::lineLineIntersection(
                    line.start(), line.end(), bLine->start(), bLine->end(), ix)) {
                // Compute t parameter
                double t;
                if (std::abs(dx) > std::abs(dy))
                    t = (ix.x - line.start().x) / dx;
                else
                    t = (ix.y - line.start().y) / dy;
                if (t > 1e-9 && t < 1.0 - 1e-9)
                    result.push_back({ix, t});
            }
        } else if (boundary->type() == EntityType::Circle) {
            auto* bCircle = static_cast<CircleEntity*>(boundary);
            Point2D h1, h2;
            int count = GeometryOps::lineCircleIntersection(
                line.start(), line.end(), bCircle->center(), bCircle->radius(), h1, h2);
            for (int i = 0; i < count; ++i) {
                const Point2D& ix = (i == 0) ? h1 : h2;
                double t;
                if (std::abs(dx) > std::abs(dy))
                    t = (ix.x - line.start().x) / dx;
                else
                    t = (ix.y - line.start().y) / dy;
                if (t > 1e-9 && t < 1.0 - 1e-9)
                    result.push_back({ix, t});
            }
        }
        // Polyline boundaries: check each segment
        else if (boundary->type() == EntityType::Polyline) {
            auto* bPoly = static_cast<PolylineEntity*>(boundary);
            const auto& verts = bPoly->vertices();
            for (size_t i = 0; i + 1 < verts.size(); ++i) {
                Point2D ix;
                if (GeometryOps::lineLineIntersection(
                        line.start(), line.end(), verts[i], verts[i + 1], ix)) {
                    double t;
                    if (std::abs(dx) > std::abs(dy))
                        t = (ix.x - line.start().x) / dx;
                    else
                        t = (ix.y - line.start().y) / dy;
                    if (t > 1e-9 && t < 1.0 - 1e-9)
                        result.push_back({ix, t});
                }
            }
            if (bPoly->isClosed() && verts.size() >= 3) {
                Point2D ix;
                if (GeometryOps::lineLineIntersection(
                        line.start(), line.end(), verts.back(), verts.front(), ix)) {
                    double t;
                    if (std::abs(dx) > std::abs(dy))
                        t = (ix.x - line.start().x) / dx;
                    else
                        t = (ix.y - line.start().y) / dy;
                    if (t > 1e-9 && t < 1.0 - 1e-9)
                        result.push_back({ix, t});
                }
            }
        }
    }

    return result;
}

} // namespace yjcad
