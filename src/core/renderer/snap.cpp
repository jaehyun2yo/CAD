#include "snap.h"
#include "core/geometry/operations.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;

SnapEngine::SnapEngine(const Document& doc) : m_doc(doc) {}

void SnapEngine::setGridSnap(bool enabled, double spacing) {
    m_gridSnap = enabled;
    m_gridSpacing = spacing;
}

SnapResult SnapEngine::snap(const Point2D& worldPos, double tolerance) const {
    SnapResult best;
    best.point = worldPos;
    double bestDist = tolerance;

    // Priority: Endpoint > Intersection > Midpoint > Perpendicular
    //           > Tangent > Quadrant > Center > Nearest > Grid
    auto tryUpdate = [&](SnapType /*type*/, const SnapResult& r) {
        if (r.valid) {
            double d = worldPos.distanceTo(r.point);
            if (d < bestDist) { best = r; bestDist = d; }
        }
    };

    if (m_endpointSnap)
        tryUpdate(SnapType::Endpoint, snapToEndpoints(worldPos, tolerance));

    if (m_intersectionSnap)
        tryUpdate(SnapType::Intersection, snapToIntersections(worldPos, tolerance));

    if (m_midpointSnap)
        tryUpdate(SnapType::Midpoint, snapToMidpoints(worldPos, tolerance));

    if (m_perpendicularSnap)
        tryUpdate(SnapType::Perpendicular, snapToPerpendicular(worldPos, tolerance));

    if (m_tangentSnap)
        tryUpdate(SnapType::Tangent, snapToTangent(worldPos, tolerance));

    if (m_quadrantSnap)
        tryUpdate(SnapType::Quadrant, snapToQuadrant(worldPos, tolerance));

    if (m_centerSnap)
        tryUpdate(SnapType::Center, snapToCenters(worldPos, tolerance));

    if (m_nearestSnap)
        tryUpdate(SnapType::Nearest, snapToNearest(worldPos, tolerance));

    // Grid snap: lowest priority, only when nothing else matched
    if (m_gridSnap && !best.valid) {
        best = snapToGrid(worldPos);
    }

    return best;
}

SnapResult SnapEngine::snapToGrid(const Point2D& pos) const {
    double gx = std::round(pos.x / m_gridSpacing) * m_gridSpacing;
    double gy = std::round(pos.y / m_gridSpacing) * m_gridSpacing;
    return {{gx, gy}, SnapType::Grid, true};
}

SnapResult SnapEngine::snapToEndpoints(const Point2D& pos, double tol) const {
    SnapResult best;
    double bestDist = tol;

    for (EntityId id : m_doc.entityIds()) {
        const Entity* e = m_doc.entity(id);
        if (!e || !e->isVisible()) continue;

        std::vector<Point2D> endpoints;

        switch (e->type()) {
            case EntityType::Line: {
                auto* l = static_cast<const LineEntity*>(e);
                endpoints.push_back(l->start());
                endpoints.push_back(l->end());
                break;
            }
            case EntityType::Polyline: {
                auto* p = static_cast<const PolylineEntity*>(e);
                endpoints = p->vertices();
                break;
            }
            case EntityType::Text: {
                auto* t = static_cast<const TextEntity*>(e);
                endpoints.push_back(t->position());
                break;
            }
            default: break;
        }

        for (const auto& ep : endpoints) {
            double d = pos.distanceTo(ep);
            if (d < bestDist) {
                bestDist = d;
                best = {ep, SnapType::Endpoint, true};
            }
        }
    }
    return best;
}

SnapResult SnapEngine::snapToMidpoints(const Point2D& pos, double tol) const {
    SnapResult best;
    double bestDist = tol;

    for (EntityId id : m_doc.entityIds()) {
        const Entity* e = m_doc.entity(id);
        if (!e || !e->isVisible()) continue;

        if (e->type() == EntityType::Line) {
            auto* l = static_cast<const LineEntity*>(e);
            Point2D mid{(l->start().x + l->end().x) / 2.0,
                        (l->start().y + l->end().y) / 2.0};
            double d = pos.distanceTo(mid);
            if (d < bestDist) {
                bestDist = d;
                best = {mid, SnapType::Midpoint, true};
            }
        }
    }
    return best;
}

SnapResult SnapEngine::snapToCenters(const Point2D& pos, double tol) const {
    SnapResult best;
    double bestDist = tol;

    for (EntityId id : m_doc.entityIds()) {
        const Entity* e = m_doc.entity(id);
        if (!e || !e->isVisible()) continue;

        Point2D center;
        bool hasCenter = false;

        if (e->type() == EntityType::Circle) {
            center = static_cast<const CircleEntity*>(e)->center();
            hasCenter = true;
        } else if (e->type() == EntityType::Arc) {
            center = static_cast<const ArcEntity*>(e)->center();
            hasCenter = true;
        }

        if (hasCenter) {
            double d = pos.distanceTo(center);
            if (d < bestDist) {
                bestDist = d;
                best = {center, SnapType::Center, true};
            }
        }
    }
    return best;
}

SnapResult SnapEngine::snapToNearest(const Point2D& pos, double tol) const {
    // Find the closest point on any entity geometry (not just special points)
    SnapResult best;
    double bestDist = tol;

    for (EntityId id : m_doc.entityIds()) {
        const Entity* e = m_doc.entity(id);
        if (!e || !e->isVisible()) continue;

        switch (e->type()) {
            case EntityType::Line: {
                auto* l = static_cast<const LineEntity*>(e);
                double d = GeometryOps::pointToSegmentDistance(pos, l->start(), l->end());
                if (d < bestDist) {
                    // Project pos onto the segment to get the nearest point
                    double dx = l->end().x - l->start().x;
                    double dy = l->end().y - l->start().y;
                    double lenSq = dx * dx + dy * dy;
                    double t = 0.0;
                    if (lenSq > 1e-18) {
                        t = std::clamp(
                            ((pos.x - l->start().x) * dx + (pos.y - l->start().y) * dy) / lenSq,
                            0.0, 1.0);
                    }
                    Point2D nearest{l->start().x + t * dx, l->start().y + t * dy};
                    bestDist = d;
                    best = {nearest, SnapType::Nearest, true};
                }
                break;
            }
            case EntityType::Polyline: {
                auto* poly = static_cast<const PolylineEntity*>(e);
                const auto& verts = poly->vertices();
                for (size_t i = 0; i + 1 < verts.size(); ++i) {
                    double d = GeometryOps::pointToSegmentDistance(pos, verts[i], verts[i + 1]);
                    if (d < bestDist) {
                        double dx = verts[i + 1].x - verts[i].x;
                        double dy = verts[i + 1].y - verts[i].y;
                        double lenSq = dx * dx + dy * dy;
                        double t = 0.0;
                        if (lenSq > 1e-18) {
                            t = std::clamp(
                                ((pos.x - verts[i].x) * dx + (pos.y - verts[i].y) * dy) / lenSq,
                                0.0, 1.0);
                        }
                        Point2D nearest{verts[i].x + t * dx, verts[i].y + t * dy};
                        bestDist = d;
                        best = {nearest, SnapType::Nearest, true};
                    }
                }
                break;
            }
            case EntityType::Circle: {
                auto* c = static_cast<const CircleEntity*>(e);
                double distToCenter = pos.distanceTo(c->center());
                if (distToCenter > 1e-12) {
                    // Nearest point on circle perimeter
                    double nx = (pos.x - c->center().x) / distToCenter;
                    double ny = (pos.y - c->center().y) / distToCenter;
                    Point2D nearest{c->center().x + nx * c->radius(),
                                    c->center().y + ny * c->radius()};
                    double d = pos.distanceTo(nearest);
                    if (d < bestDist) {
                        bestDist = d;
                        best = {nearest, SnapType::Nearest, true};
                    }
                }
                break;
            }
            case EntityType::Arc: {
                auto* a = static_cast<const ArcEntity*>(e);
                double distToCenter = pos.distanceTo(a->center());
                if (distToCenter > 1e-12) {
                    double angle = std::atan2(pos.y - a->center().y,
                                             pos.x - a->center().x) * 180.0 / PI;
                    if (angle < 0) angle += 360.0;
                    // Clamp angle to arc range
                    double sa = a->startAngle();
                    double ea = a->endAngle();
                    if (ea < sa) ea += 360.0;
                    double angleNorm = angle;
                    if (angleNorm < sa) angleNorm += 360.0;
                    double clampedAngle;
                    if (angleNorm >= sa && angleNorm <= ea) {
                        clampedAngle = angle;
                    } else {
                        // Clamp to closer endpoint
                        double dStart = std::abs(angleNorm - sa);
                        double dEnd   = std::abs(angleNorm - ea);
                        clampedAngle = (dStart < dEnd) ? sa : a->endAngle();
                    }
                    double rad = clampedAngle * PI / 180.0;
                    Point2D nearest{a->center().x + a->radius() * std::cos(rad),
                                    a->center().y + a->radius() * std::sin(rad)};
                    double d = pos.distanceTo(nearest);
                    if (d < bestDist) {
                        bestDist = d;
                        best = {nearest, SnapType::Nearest, true};
                    }
                }
                break;
            }
            default: break;
        }
    }
    return best;
}

SnapResult SnapEngine::snapToIntersections(const Point2D& pos, double tol) const {
    SnapResult best;
    double bestDist = tol;

    const auto& ids = m_doc.entityIds();
    for (size_t i = 0; i < ids.size(); ++i) {
        const Entity* eA = m_doc.entity(ids[i]);
        if (!eA || !eA->isVisible()) continue;

        for (size_t j = i + 1; j < ids.size(); ++j) {
            const Entity* eB = m_doc.entity(ids[j]);
            if (!eB || !eB->isVisible()) continue;

            // Collect candidate segment pairs for A
            std::vector<std::pair<Point2D, Point2D>> segsA, segsB;

            auto collectLineSegs = [](const Entity* e,
                                       std::vector<std::pair<Point2D, Point2D>>& segs) {
                switch (e->type()) {
                    case EntityType::Line: {
                        auto* l = static_cast<const LineEntity*>(e);
                        segs.push_back({l->start(), l->end()});
                        break;
                    }
                    case EntityType::Polyline: {
                        auto* p = static_cast<const PolylineEntity*>(e);
                        const auto& v = p->vertices();
                        for (size_t k = 0; k + 1 < v.size(); ++k)
                            segs.push_back({v[k], v[k + 1]});
                        break;
                    }
                    default: break;
                }
            };

            collectLineSegs(eA, segsA);
            collectLineSegs(eB, segsB);

            // Line-line intersections
            for (const auto& sA : segsA) {
                for (const auto& sB : segsB) {
                    Point2D hit;
                    if (GeometryOps::lineLineIntersection(
                            sA.first, sA.second, sB.first, sB.second, hit)) {
                        double d = pos.distanceTo(hit);
                        if (d < bestDist) {
                            bestDist = d;
                            best = {hit, SnapType::Intersection, true};
                        }
                    }
                }
            }

            // Line-circle and line-arc intersections
            auto tryLineCircle = [&](const std::pair<Point2D, Point2D>& seg,
                                     const Point2D& center, double radius) {
                Point2D h1, h2;
                int cnt = GeometryOps::lineCircleIntersection(
                    seg.first, seg.second, center, radius, h1, h2);
                for (int k = 0; k < cnt; ++k) {
                    const Point2D& h = (k == 0) ? h1 : h2;
                    double d = pos.distanceTo(h);
                    if (d < bestDist) {
                        bestDist = d;
                        best = {h, SnapType::Intersection, true};
                    }
                }
            };

            // A=line/poly vs B=circle/arc
            auto processCircle = [&](const Entity* lineEnt, const Entity* circEnt) {
                std::vector<std::pair<Point2D, Point2D>> lineParts;
                collectLineSegs(lineEnt, lineParts);
                Point2D ctr;
                double  rad = 0.0;
                bool isCurve = false;
                if (circEnt->type() == EntityType::Circle) {
                    auto* c = static_cast<const CircleEntity*>(circEnt);
                    ctr = c->center(); rad = c->radius(); isCurve = true;
                } else if (circEnt->type() == EntityType::Arc) {
                    auto* a = static_cast<const ArcEntity*>(circEnt);
                    ctr = a->center(); rad = a->radius(); isCurve = true;
                }
                if (isCurve) {
                    for (const auto& seg : lineParts)
                        tryLineCircle(seg, ctr, rad);
                }
            };

            processCircle(eA, eB);
            processCircle(eB, eA);
        }
    }
    return best;
}

SnapResult SnapEngine::snapToPerpendicular(const Point2D& pos, double tol) const {
    // Requires a reference point (last committed drawing point)
    if (!m_hasRef) return {};

    SnapResult best;
    double bestDist = tol;

    for (EntityId id : m_doc.entityIds()) {
        const Entity* e = m_doc.entity(id);
        if (!e || !e->isVisible()) continue;

        if (e->type() == EntityType::Line) {
            auto* l = static_cast<const LineEntity*>(e);
            double dx = l->end().x - l->start().x;
            double dy = l->end().y - l->start().y;
            double lenSq = dx * dx + dy * dy;
            if (lenSq < 1e-18) continue;

            // Project m_refPoint onto the infinite line, then foot is perp point
            double t = ((m_refPoint.x - l->start().x) * dx +
                        (m_refPoint.y - l->start().y) * dy) / lenSq;
            t = std::clamp(t, 0.0, 1.0);
            Point2D foot{l->start().x + t * dx, l->start().y + t * dy};
            double d = pos.distanceTo(foot);
            if (d < bestDist) {
                bestDist = d;
                best = {foot, SnapType::Perpendicular, true};
            }
        } else if (e->type() == EntityType::Polyline) {
            auto* poly = static_cast<const PolylineEntity*>(e);
            const auto& verts = poly->vertices();
            for (size_t k = 0; k + 1 < verts.size(); ++k) {
                double dx = verts[k + 1].x - verts[k].x;
                double dy = verts[k + 1].y - verts[k].y;
                double lenSq = dx * dx + dy * dy;
                if (lenSq < 1e-18) continue;
                double t = std::clamp(
                    ((m_refPoint.x - verts[k].x) * dx +
                     (m_refPoint.y - verts[k].y) * dy) / lenSq,
                    0.0, 1.0);
                Point2D foot{verts[k].x + t * dx, verts[k].y + t * dy};
                double d = pos.distanceTo(foot);
                if (d < bestDist) {
                    bestDist = d;
                    best = {foot, SnapType::Perpendicular, true};
                }
            }
        }
    }
    return best;
}

SnapResult SnapEngine::snapToTangent(const Point2D& pos, double tol) const {
    // Requires a reference point — finds point on circle/arc where the
    // line from m_refPoint is tangent
    if (!m_hasRef) return {};

    SnapResult best;
    double bestDist = tol;

    for (EntityId id : m_doc.entityIds()) {
        const Entity* e = m_doc.entity(id);
        if (!e || !e->isVisible()) continue;

        Point2D center;
        double  radius = 0.0;
        bool isCurve = false;

        if (e->type() == EntityType::Circle) {
            auto* c = static_cast<const CircleEntity*>(e);
            center = c->center(); radius = c->radius(); isCurve = true;
        } else if (e->type() == EntityType::Arc) {
            auto* a = static_cast<const ArcEntity*>(e);
            center = a->center(); radius = a->radius(); isCurve = true;
        }

        if (!isCurve) continue;

        // Distance from reference point to circle center
        double distRef = m_refPoint.distanceTo(center);
        if (distRef < radius - 1e-9) continue; // ref point is inside — no tangent

        // Tangent length from ref point to tangent point
        double tangentLen = std::sqrt(std::max(0.0, distRef * distRef - radius * radius));

        // Angle of ref→center
        double angleToCenter = std::atan2(center.y - m_refPoint.y,
                                          center.x - m_refPoint.x);
        // Half-angle of tangent
        double halfAngle = std::asin(std::min(1.0, radius / distRef));

        // Two tangent points
        for (int sign : {-1, 1}) {
            double tangentAngle = angleToCenter + sign * halfAngle;
            // Tangent foot on circle
            // foot = ref + tangentLen * (cos, sin) direction to tangent point
            // Alternatively: tangent point on circle at angle (tangentAngle - PI/2 * sign) from center
            double footAngle = std::atan2(
                m_refPoint.y + tangentLen * std::sin(tangentAngle) - center.y,
                m_refPoint.x + tangentLen * std::cos(tangentAngle) - center.x);
            Point2D tangentPt{center.x + radius * std::cos(footAngle),
                              center.y + radius * std::sin(footAngle)};

            // Check if within arc range (for arc entities)
            if (e->type() == EntityType::Arc) {
                auto* a = static_cast<const ArcEntity*>(e);
                double angleDeg = footAngle * 180.0 / PI;
                if (angleDeg < 0) angleDeg += 360.0;
                double sa = a->startAngle();
                double ea = a->endAngle();
                if (ea < sa) ea += 360.0;
                double norm = angleDeg;
                if (norm < sa) norm += 360.0;
                if (norm < sa || norm > ea) continue;
            }

            double d = pos.distanceTo(tangentPt);
            if (d < bestDist) {
                bestDist = d;
                best = {tangentPt, SnapType::Tangent, true};
            }
        }
    }
    return best;
}

SnapResult SnapEngine::snapToQuadrant(const Point2D& pos, double tol) const {
    // Snaps to 0°, 90°, 180°, 270° points on circles and arcs
    SnapResult best;
    double bestDist = tol;

    static const double quadAngles[4] = {0.0, 90.0, 180.0, 270.0};

    for (EntityId id : m_doc.entityIds()) {
        const Entity* e = m_doc.entity(id);
        if (!e || !e->isVisible()) continue;

        Point2D center;
        double  radius = 0.0;
        double  startAngleDeg = 0.0;
        double  endAngleDeg   = 360.0;
        bool    checkArcRange = false;
        bool    isCurve = false;

        if (e->type() == EntityType::Circle) {
            auto* c = static_cast<const CircleEntity*>(e);
            center = c->center(); radius = c->radius(); isCurve = true;
        } else if (e->type() == EntityType::Arc) {
            auto* a = static_cast<const ArcEntity*>(e);
            center = a->center(); radius = a->radius();
            startAngleDeg = a->startAngle();
            endAngleDeg   = a->endAngle();
            checkArcRange = true;
            isCurve = true;
        }

        if (!isCurve) continue;

        double ea = endAngleDeg;
        if (checkArcRange && ea < startAngleDeg) ea += 360.0;

        for (double qa : quadAngles) {
            if (checkArcRange) {
                // Check if this quadrant angle is within the arc
                double norm = qa;
                if (norm < startAngleDeg) norm += 360.0;
                if (norm < startAngleDeg || norm > ea) continue;
            }
            double rad = qa * PI / 180.0;
            Point2D qpt{center.x + radius * std::cos(rad),
                        center.y + radius * std::sin(rad)};
            double d = pos.distanceTo(qpt);
            if (d < bestDist) {
                bestDist = d;
                best = {qpt, SnapType::Quadrant, true};
            }
        }
    }
    return best;
}

} // namespace yjcad
