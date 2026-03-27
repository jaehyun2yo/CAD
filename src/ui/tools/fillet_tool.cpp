#include "fillet_tool.h"
#include "core/document/document.h"
#include "core/geometry/operations.h"
#include "ui/cad_canvas.h"
#include <cmath>
#include <algorithm>

namespace yjcad {

static constexpr double kPiF = 3.14159265358979323846;

/// Pick the closest line entity to a world position
static EntityId pickLineEntity(Document* doc, const Point2D& worldPos) {
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

bool FilletTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_firstSelected) {
        EntityId id = pickLineEntity(m_doc, worldPos);
        if (id == 0) return false;
        m_firstId       = id;
        m_firstClick    = worldPos;
        m_firstSelected = true;

        auto* line = static_cast<LineEntity*>(m_doc->entity(id));
        if (line) m_preview.push_back({line->start(), line->end()});
        return true;
    }

    EntityId id2 = pickLineEntity(m_doc, worldPos);
    if (id2 == 0 || id2 == m_firstId) return false;

    executeFillet(m_firstId, m_firstClick, id2, worldPos);
    cancel();
    return true;
}

bool FilletTool::mouseMove(const Point2D& worldPos) {
    (void)worldPos;
    return false;
}

bool FilletTool::keyPress(int key) {
    if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
        m_radius += 1.0;
        return true;
    }
    if (key == Qt::Key_Minus) {
        m_radius = std::max(m_radius - 1.0, 0.5);
        return true;
    }
    return false;
}

void FilletTool::executeFillet(EntityId id1, const Point2D& click1,
                                EntityId id2, const Point2D& click2) {
    auto* e1 = m_doc->entity(id1);
    auto* e2 = m_doc->entity(id2);
    if (!e1 || !e2) return;
    if (e1->type() != EntityType::Line || e2->type() != EntityType::Line) return;

    auto* line1 = static_cast<LineEntity*>(e1);
    auto* line2 = static_cast<LineEntity*>(e2);

    double len1 = line1->length();
    double len2 = line2->length();
    if (len1 < 1e-12 || len2 < 1e-12) return;

    // Find corner (infinite-line intersection)
    auto extFactor = [](const LineEntity& l, double factor) {
        double dx = l.end().x - l.start().x;
        double dy = l.end().y - l.start().y;
        return Point2D{l.start().x + dx * factor, l.start().y + dy * factor};
    };

    Point2D a1 = extFactor(*line1, -10.0);
    Point2D a2 = extFactor(*line1,  10.0);
    Point2D b1 = extFactor(*line2, -10.0);
    Point2D b2 = extFactor(*line2,  10.0);

    Point2D corner;
    if (!GeometryOps::lineLineIntersection(a1, a2, b1, b2, corner)) return;

    // Unit vectors of each line pointing away from corner
    double dx1 = line1->end().x - line1->start().x;
    double dy1 = line1->end().y - line1->start().y;
    double dx2 = line2->end().x - line2->start().x;
    double dy2 = line2->end().y - line2->start().y;

    // Determine which end of each line faces the corner (click determines this)
    bool end1near = (click1.distanceTo(line1->end()) < click1.distanceTo(line1->start()));
    bool end2near = (click2.distanceTo(line2->end()) < click2.distanceTo(line2->start()));

    // Direction from corner outward along each line
    double outX1 = end1near ? -dx1 / len1 :  dx1 / len1;
    double outY1 = end1near ? -dy1 / len1 :  dy1 / len1;
    double outX2 = end2near ? -dx2 / len2 :  dx2 / len2;
    double outY2 = end2near ? -dy2 / len2 :  dy2 / len2;

    // Half-angle between the two directions
    double dot = outX1 * outX2 + outY1 * outY2;
    dot = std::clamp(dot, -1.0, 1.0);
    double halfAngle = std::acos(dot) / 2.0;
    if (halfAngle < 1e-9) return;

    // Distance from corner to tangent points
    double tanDist = m_radius / std::tan(halfAngle);
    if (tanDist < 1e-9) return;

    // Tangent cut points on each line
    Point2D cut1{corner.x + outX1 * tanDist, corner.y + outY1 * tanDist};
    Point2D cut2{corner.x + outX2 * tanDist, corner.y + outY2 * tanDist};

    // Arc center: perpendicular from each tangent point by radius
    // Bisector direction (normalized)
    double bisX = outX1 + outX2;
    double bisY = outY1 + outY2;
    double bisLen = std::sqrt(bisX * bisX + bisY * bisY);
    if (bisLen < 1e-9) return;
    bisX /= bisLen; bisY /= bisLen;

    // Distance from corner to arc center
    double centerDist = m_radius / std::sin(halfAngle);
    Point2D arcCenter{corner.x + bisX * centerDist, corner.y + bisY * centerDist};

    // Compute arc start and end angles
    double startAng = std::atan2(cut1.y - arcCenter.y, cut1.x - arcCenter.x) * 180.0 / kPiF;
    double endAng   = std::atan2(cut2.y - arcCenter.y, cut2.x - arcCenter.x) * 180.0 / kPiF;

    // Normalize angles to [0, 360)
    auto norm360 = [](double a) {
        while (a < 0.0)   a += 360.0;
        while (a >= 360.0) a -= 360.0;
        return a;
    };
    startAng = norm360(startAng);
    endAng   = norm360(endAng);

    // Ensure the arc sweeps through the correct side (toward the corner)
    // The arc should sweep the short way between the two tangent angles
    // through the region away from the corner (concave arc toward corner)
    double sweep = endAng - startAng;
    if (sweep < 0.0) sweep += 360.0;
    if (sweep > 180.0) {
        // Swap to take the short arc
        std::swap(startAng, endAng);
    }

    // Capture meta before removing
    std::string layer1 = line1->layerName(); int color1 = line1->color(); auto cl1 = line1->classification();
    std::string layer2 = line2->layerName(); int color2 = line2->color(); auto cl2 = line2->classification();

    // New trimmed lines
    Point2D newStart1 = end1near ? line1->start() : cut1;
    Point2D newEnd1   = end1near ? cut1            : line1->end();
    Point2D newStart2 = end2near ? line2->start() : cut2;
    Point2D newEnd2   = end2near ? cut2            : line2->end();

    // Remove originals
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

    // Add fillet arc
    auto arc = std::make_unique<ArcEntity>(m_doc->nextId(), arcCenter, m_radius, startAng, endAng);
    arc->setLayerName(layer1); arc->setColor(color1); arc->setClassification(cl1);
    m_doc->history().execute(std::make_unique<AddEntityCommand>(*m_doc, std::move(arc)));

    if (m_canvas) m_canvas->refreshEntities();
}

void FilletTool::cancel() {
    m_firstSelected = false;
    m_firstId = 0;
    m_preview.clear();
}

} // namespace yjcad
