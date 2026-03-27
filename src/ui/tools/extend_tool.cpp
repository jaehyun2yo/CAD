#include "extend_tool.h"
#include "core/document/document.h"
#include "core/geometry/operations.h"
#include "ui/cad_canvas.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace yjcad {

bool ExtendTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    // Phase 1: capture boundary selection
    if (!m_boundariesSet) {
        auto selected = m_doc->selection().selectedIds();
        m_boundaryIds.assign(selected.begin(), selected.end());
        if (m_boundaryIds.empty()) return false;
        m_boundariesSet = true;
        m_doc->selection().clear();
        return true;
    }

    // Phase 2+: find the closest line entity to click, determine which end to extend
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

    // Determine which endpoint is closer to the click point
    double dStart = worldPos.distanceTo(line->start());
    double dEnd   = worldPos.distanceTo(line->end());
    bool extendEnd = (dEnd < dStart);

    Point2D target;
    if (!findExtendTarget(*line, extendEnd, target)) return false;

    // Build new extended line
    Point2D newStart = extendEnd ? line->start() : target;
    Point2D newEnd   = extendEnd ? target         : line->end();

    EntityId newId = m_doc->nextId();
    auto rmCmd = std::make_unique<RemoveEntityCommand>(*m_doc, closestId);
    m_doc->history().execute(std::move(rmCmd));

    auto newLine = std::make_unique<LineEntity>(newId, newStart, newEnd);
    newLine->setLayerName(line->layerName());
    newLine->setColor(line->color());
    newLine->setClassification(line->classification());
    auto addCmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(newLine));
    m_doc->history().execute(std::move(addCmd));

    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool ExtendTool::mouseMove(const Point2D& worldPos) {
    (void)worldPos;
    return false;
}

bool ExtendTool::findExtendTarget(const LineEntity& line, bool extendEnd,
                                  Point2D& outTarget) {
    // Direction vector of the line
    double dx = line.end().x - line.start().x;
    double dy = line.end().y - line.start().y;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-12) return false;

    // The endpoint we are extending from
    const Point2D& endPt = extendEnd ? line.end() : line.start();

    // Create an extended ray from the endpoint in the extension direction
    // For extendEnd: direction is from start → end, extended beyond end
    // For extendStart: direction is from end → start, extended beyond start
    double dirX = extendEnd ?  dx / len : -dx / len;
    double dirY = extendEnd ?  dy / len : -dy / len;

    // Use a very long ray for intersection testing
    double rayLen = 1e6;
    Point2D rayEnd{endPt.x + dirX * rayLen, endPt.y + dirY * rayLen};

    double bestT = std::numeric_limits<double>::max();
    bool found = false;

    for (EntityId bid : m_boundaryIds) {
        auto* boundary = m_doc->entity(bid);
        if (!boundary) continue;

        if (boundary->type() == EntityType::Line) {
            auto* bLine = static_cast<LineEntity*>(boundary);
            Point2D ix;
            if (GeometryOps::lineLineIntersection(endPt, rayEnd,
                    bLine->start(), bLine->end(), ix)) {
                // Compute t along the ray
                double t = (std::abs(dirX) > std::abs(dirY))
                    ? (ix.x - endPt.x) / dirX
                    : (ix.y - endPt.y) / dirY;
                if (t > 1e-9 && t < bestT) {
                    bestT = t;
                    outTarget = ix;
                    found = true;
                }
            }
        }
        // Circle boundary
        else if (boundary->type() == EntityType::Circle) {
            auto* bCircle = static_cast<CircleEntity*>(boundary);
            Point2D h1, h2;
            int count = GeometryOps::lineCircleIntersection(
                endPt, rayEnd, bCircle->center(), bCircle->radius(), h1, h2);
            for (int i = 0; i < count; ++i) {
                const Point2D& ix = (i == 0) ? h1 : h2;
                double t = (std::abs(dirX) > std::abs(dirY))
                    ? (ix.x - endPt.x) / dirX
                    : (ix.y - endPt.y) / dirY;
                if (t > 1e-9 && t < bestT) {
                    bestT = t;
                    outTarget = ix;
                    found = true;
                }
            }
        }
    }

    return found;
}

void ExtendTool::cancel() {
    m_boundaryIds.clear();
    m_boundariesSet = false;
    m_preview.clear();
}

} // namespace yjcad
