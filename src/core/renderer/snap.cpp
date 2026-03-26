#include "snap.h"
#include <cmath>
#include <limits>

namespace yjcad {

SnapEngine::SnapEngine(const Document& doc) : m_doc(doc) {}

void SnapEngine::setGridSnap(bool enabled, double spacing) {
    m_gridSnap = enabled;
    m_gridSpacing = spacing;
}

SnapResult SnapEngine::snap(const Point2D& worldPos, double tolerance) const {
    SnapResult best;
    best.point = worldPos;
    double bestDist = tolerance;

    // Priority: Endpoint > Midpoint > Center > Grid
    if (m_endpointSnap) {
        auto r = snapToEndpoints(worldPos, tolerance);
        if (r.valid) {
            double d = worldPos.distanceTo(r.point);
            if (d < bestDist) { best = r; bestDist = d; }
        }
    }

    if (m_midpointSnap) {
        auto r = snapToMidpoints(worldPos, tolerance);
        if (r.valid) {
            double d = worldPos.distanceTo(r.point);
            if (d < bestDist) { best = r; bestDist = d; }
        }
    }

    if (m_centerSnap) {
        auto r = snapToCenters(worldPos, tolerance);
        if (r.valid) {
            double d = worldPos.distanceTo(r.point);
            if (d < bestDist) { best = r; bestDist = d; }
        }
    }

    // Grid snap (lowest priority, but always available)
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

} // namespace yjcad
