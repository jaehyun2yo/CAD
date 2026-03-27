#include "offset_tool.h"
#include "core/document/document.h"
#include "core/geometry/operations.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

bool OffsetTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_entitySelected) {
        // First: need a selected entity (polyline or line)
        auto selected = m_doc->selection().selectedIds();
        if (selected.empty()) return false;

        // Use first selected entity
        m_targetId = *selected.begin();
        auto* entity = m_doc->entity(m_targetId);
        if (!entity) return false;

        // Only polyline and line support offset
        if (entity->type() != EntityType::Polyline && entity->type() != EntityType::Line)
            return false;

        m_entitySelected = true;
        return true;
    }

    // Second click: compute offset distance and direction
    auto* entity = m_doc->entity(m_targetId);
    if (!entity) { cancel(); return false; }

    if (entity->type() == EntityType::Polyline) {
        auto* poly = static_cast<PolylineEntity*>(entity);

        // Find closest point on polyline to determine which side
        double minDist = 1e18;
        Point2D closest;
        const auto& verts = poly->vertices();
        for (size_t i = 0; i + 1 < verts.size(); ++i) {
            double d = GeometryOps::pointToSegmentDistance(worldPos, verts[i], verts[i + 1]);
            if (d < minDist) {
                minDist = d;
            }
        }
        if (poly->isClosed() && verts.size() >= 3) {
            double d = GeometryOps::pointToSegmentDistance(worldPos, verts.back(), verts.front());
            if (d < minDist) minDist = d;
        }

        // Use distance as offset amount, determine sign by side
        double distance = minDist;
        // Try positive offset first, check if result is closer to click point
        auto posResult = GeometryOps::offsetPolyline(*poly, distance, m_doc->nextId());
        auto negResult = GeometryOps::offsetPolyline(*poly, -distance, m_doc->nextId());

        std::unique_ptr<PolylineEntity> result;
        if (posResult && negResult) {
            // Pick the one closer to the click point
            double posDist = 1e18, negDist = 1e18;
            for (const auto& v : posResult->vertices()) {
                posDist = std::min(posDist, worldPos.distanceTo(v));
            }
            for (const auto& v : negResult->vertices()) {
                negDist = std::min(negDist, worldPos.distanceTo(v));
            }
            result = (posDist < negDist) ? std::move(posResult) : std::move(negResult);
        } else if (posResult) {
            result = std::move(posResult);
        } else if (negResult) {
            result = std::move(negResult);
        }

        if (result) {
            auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(result));
            m_doc->history().execute(std::move(cmd));
        }
    } else if (entity->type() == EntityType::Line) {
        auto* line = static_cast<LineEntity*>(entity);
        // Convert line to 2-vertex polyline, offset, then add result
        std::vector<Point2D> verts = {line->start(), line->end()};
        PolylineEntity tempPoly(0, verts, false);
        tempPoly.setLayerName(line->layerName());
        tempPoly.setColor(line->color());
        tempPoly.setClassification(line->classification());

        double dist = GeometryOps::pointToSegmentDistance(worldPos,
                                                           line->start(), line->end());
        auto posResult = GeometryOps::offsetPolyline(tempPoly, dist, m_doc->nextId());
        auto negResult = GeometryOps::offsetPolyline(tempPoly, -dist, m_doc->nextId());

        std::unique_ptr<PolylineEntity> result;
        if (posResult && negResult) {
            double posDist = 1e18, negDist = 1e18;
            for (const auto& v : posResult->vertices())
                posDist = std::min(posDist, worldPos.distanceTo(v));
            for (const auto& v : negResult->vertices())
                negDist = std::min(negDist, worldPos.distanceTo(v));
            result = (posDist < negDist) ? std::move(posResult) : std::move(negResult);
        } else if (posResult) {
            result = std::move(posResult);
        } else if (negResult) {
            result = std::move(negResult);
        }

        if (result) {
            auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(result));
            m_doc->history().execute(std::move(cmd));
        }
    }

    m_entitySelected = false;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool OffsetTool::mouseMove(const Point2D& worldPos) {
    (void)worldPos;
    return false;
}

void OffsetTool::cancel() {
    m_entitySelected = false;
    m_preview.clear();
}

} // namespace yjcad
