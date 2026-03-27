#include "chamfer_tool.h"
#include "core/document/document.h"
#include "core/geometry/operations.h"
#include "ui/cad_canvas.h"
#include <cmath>
#include <algorithm>

namespace yjcad {

/// Helper: find closest line entity to a click point, return its ID and segment distance
static EntityId pickLine(Document* doc, const Point2D& worldPos) {
    EntityId closestId = 0;
    double closestDist = 1e18;
    for (EntityId id : doc->entityIds()) {
        auto* entity = doc->entity(id);
        if (!entity || !entity->isVisible()) continue;
        if (entity->type() != EntityType::Line) continue;
        auto* line = static_cast<LineEntity*>(entity);
        double d = GeometryOps::pointToSegmentDistance(worldPos, line->start(), line->end());
        if (d < closestDist) {
            closestDist = d;
            closestId = id;
        }
    }
    return closestId;
}

/// Compute t parameter for the endpoint nearest to clickPos on the line
static double nearestEndT(const LineEntity& line, const Point2D& clickPos) {
    double dStart = clickPos.distanceTo(line.start());
    double dEnd   = clickPos.distanceTo(line.end());
    return (dEnd < dStart) ? 1.0 : 0.0;
}

bool ChamferTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_firstSelected) {
        EntityId id = pickLine(m_doc, worldPos);
        if (id == 0) return false;
        m_firstId       = id;
        m_firstClick    = worldPos;
        m_firstSelected = true;

        // Preview: highlight first line
        auto* line = static_cast<LineEntity*>(m_doc->entity(id));
        if (line) m_preview.push_back({line->start(), line->end()});
        return true;
    }

    // Second click
    EntityId id2 = pickLine(m_doc, worldPos);
    if (id2 == 0 || id2 == m_firstId) return false;

    executeChamfer(m_firstId, m_firstClick, id2, worldPos);
    cancel();
    return true;
}

bool ChamferTool::mouseMove(const Point2D& worldPos) {
    (void)worldPos;
    return false;
}

bool ChamferTool::keyPress(int key) {
    if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
        m_distance += 1.0;
        return true;
    }
    if (key == Qt::Key_Minus) {
        m_distance = std::max(m_distance - 1.0, 0.5);
        return true;
    }
    return false;
}

void ChamferTool::executeChamfer(EntityId id1, const Point2D& click1,
                                  EntityId id2, const Point2D& click2) {
    auto* e1 = m_doc->entity(id1);
    auto* e2 = m_doc->entity(id2);
    if (!e1 || !e2) return;
    if (e1->type() != EntityType::Line || e2->type() != EntityType::Line) return;

    auto* line1 = static_cast<LineEntity*>(e1);
    auto* line2 = static_cast<LineEntity*>(e2);

    // Find infinite-line intersection of the two lines
    Point2D corner;
    // Use long rays to find intersection even if segments don't overlap
    double len1 = line1->length();
    double len2 = line2->length();
    if (len1 < 1e-12 || len2 < 1e-12) return;

    // Extend both lines far beyond their endpoints for intersection
    auto extendedEnd1 = [&](const LineEntity& l, double factor) {
        double dx = l.end().x - l.start().x;
        double dy = l.end().y - l.start().y;
        return Point2D{l.start().x + dx * factor, l.start().y + dy * factor};
    };

    Point2D a1 = extendedEnd1(*line1, -10.0);
    Point2D a2 = extendedEnd1(*line1,  10.0);
    Point2D b1 = extendedEnd1(*line2, -10.0);
    Point2D b2 = extendedEnd1(*line2,  10.0);

    if (!GeometryOps::lineLineIntersection(a1, a2, b1, b2, corner)) return;

    // Determine which end of each line is near the corner
    // The click determines which end the user wants to chamfer
    double t1 = nearestEndT(*line1, click1); // 0.0 = start, 1.0 = end
    double t2 = nearestEndT(*line2, click2);

    // Direction on line1 towards the corner (from far end towards corner)
    double dx1 = line1->end().x - line1->start().x;
    double dy1 = line1->end().y - line1->start().y;
    double unitX1 = dx1 / len1;
    double unitY1 = dy1 / len1;
    // If near end is "end" (t1==1), direction toward corner is +unit; else -unit
    double sign1 = (t1 > 0.5) ? 1.0 : -1.0;

    double dx2 = line2->end().x - line2->start().x;
    double dy2 = line2->end().y - line2->start().y;
    double unitX2 = dx2 / len2;
    double unitY2 = dy2 / len2;
    double sign2 = (t2 > 0.5) ? 1.0 : -1.0;

    // Chamfer cut points: move m_distance back along each line from corner
    Point2D cut1{corner.x - sign1 * unitX1 * m_distance,
                 corner.y - sign1 * unitY1 * m_distance};
    Point2D cut2{corner.x - sign2 * unitX2 * m_distance,
                 corner.y - sign2 * unitY2 * m_distance};

    // New endpoints for line1 (the end away from corner stays, near end becomes cut1)
    Point2D newStart1 = (t1 > 0.5) ? line1->start() : cut1;
    Point2D newEnd1   = (t1 > 0.5) ? cut1            : line1->end();

    Point2D newStart2 = (t2 > 0.5) ? line2->start() : cut2;
    Point2D newEnd2   = (t2 > 0.5) ? cut2            : line2->end();

    // Capture meta before removing
    std::string layer1 = line1->layerName(); int color1 = line1->color(); auto cl1 = line1->classification();
    std::string layer2 = line2->layerName(); int color2 = line2->color(); auto cl2 = line2->classification();

    // Remove both originals
    m_doc->history().execute(std::make_unique<RemoveEntityCommand>(*m_doc, id1));
    m_doc->history().execute(std::make_unique<RemoveEntityCommand>(*m_doc, id2));

    // Add trimmed line1
    auto nl1 = std::make_unique<LineEntity>(m_doc->nextId(), newStart1, newEnd1);
    nl1->setLayerName(layer1); nl1->setColor(color1); nl1->setClassification(cl1);
    m_doc->history().execute(std::make_unique<AddEntityCommand>(*m_doc, std::move(nl1)));

    // Add trimmed line2
    auto nl2 = std::make_unique<LineEntity>(m_doc->nextId(), newStart2, newEnd2);
    nl2->setLayerName(layer2); nl2->setColor(color2); nl2->setClassification(cl2);
    m_doc->history().execute(std::make_unique<AddEntityCommand>(*m_doc, std::move(nl2)));

    // Add chamfer connecting line (cut1 → cut2)
    auto chamferLine = std::make_unique<LineEntity>(m_doc->nextId(), cut1, cut2);
    chamferLine->setLayerName(layer1); chamferLine->setColor(color1); chamferLine->setClassification(cl1);
    m_doc->history().execute(std::make_unique<AddEntityCommand>(*m_doc, std::move(chamferLine)));

    if (m_canvas) m_canvas->refreshEntities();
}

void ChamferTool::cancel() {
    m_firstSelected = false;
    m_firstId = 0;
    m_preview.clear();
}

} // namespace yjcad
