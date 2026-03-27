#include "optimizer.h"
#include "core/document/document.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <unordered_set>

static constexpr double PI = 3.14159265358979323846;

namespace yjcad {

// ── Optimizer ──

Optimizer::Optimizer(Document& doc)
    : m_doc(doc)
{}

// ── clean ──

int Optimizer::clean(double minLen) {
    auto ids = m_doc.entityIds();
    std::vector<EntityId> toRemove;

    for (EntityId id : ids) {
        Entity* ent = m_doc.entity(id);
        if (!ent) continue;

        if (ent->type() == EntityType::Line) {
            auto* line = static_cast<LineEntity*>(ent);
            if (line->length() < minLen) {
                toRemove.push_back(id);
            }
        } else if (ent->type() == EntityType::Polyline) {
            auto* poly = static_cast<PolylineEntity*>(ent);
            // Compute approximate length of all segments
            const auto& verts = poly->vertices();
            double totalLen = 0.0;
            for (size_t i = 0; i + 1 < verts.size(); ++i) {
                totalLen += verts[i].distanceTo(verts[i + 1]);
            }
            if (poly->isClosed() && verts.size() >= 2) {
                totalLen += verts.back().distanceTo(verts.front());
            }
            if (totalLen < minLen) {
                toRemove.push_back(id);
            }
        } else if (ent->type() == EntityType::Circle) {
            auto* circle = static_cast<CircleEntity*>(ent);
            if (circle->radius() * 2.0 * PI < minLen) {
                toRemove.push_back(id);
            }
        }
    }

    for (EntityId id : toRemove) {
        m_doc.removeEntity(id);
    }

    return static_cast<int>(toRemove.size());
}

// ── findOverlaps ──

std::vector<EntityId> Optimizer::findOverlaps(double tolerance) {
    auto ids = m_doc.entityIds();
    std::unordered_set<EntityId> duplicates;

    for (size_t i = 0; i < ids.size(); ++i) {
        if (duplicates.count(ids[i])) continue;
        Entity* a = m_doc.entity(ids[i]);
        if (!a) continue;

        for (size_t j = i + 1; j < ids.size(); ++j) {
            if (duplicates.count(ids[j])) continue;
            Entity* b = m_doc.entity(ids[j]);
            if (!b) continue;
            if (a->type() != b->type()) continue;

            bool isOverlap = false;

            if (a->type() == EntityType::Line) {
                auto* la = static_cast<LineEntity*>(a);
                auto* lb = static_cast<LineEntity*>(b);
                // Same endpoints in either order
                bool sameDir = la->start().distanceTo(lb->start()) < tolerance &&
                               la->end().distanceTo(lb->end()) < tolerance;
                bool revDir  = la->start().distanceTo(lb->end()) < tolerance &&
                               la->end().distanceTo(lb->start()) < tolerance;
                isOverlap = sameDir || revDir;
            } else if (a->type() == EntityType::Circle) {
                auto* ca = static_cast<CircleEntity*>(a);
                auto* cb = static_cast<CircleEntity*>(b);
                isOverlap = ca->center().distanceTo(cb->center()) < tolerance &&
                            std::abs(ca->radius() - cb->radius()) < tolerance;
            } else if (a->type() == EntityType::Point) {
                auto boxA = a->boundingBox();
                auto boxB = b->boundingBox();
                isOverlap = boxA.min.distanceTo(boxB.min) < tolerance;
            }

            if (isOverlap) {
                duplicates.insert(ids[j]);
            }
        }
    }

    return std::vector<EntityId>(duplicates.begin(), duplicates.end());
}

// ── removeOverlaps ──

int Optimizer::removeOverlaps(double tolerance) {
    auto overlaps = findOverlaps(tolerance);
    for (EntityId id : overlaps) {
        m_doc.removeEntity(id);
    }
    return static_cast<int>(overlaps.size());
}

// ── snapJoin ──

int Optimizer::snapJoin(double gap) {
    auto ids = m_doc.entityIds();
    int snapped = 0;

    // Collect all line endpoints: {entityId, isEnd (false=start, true=end), point}
    struct EndPoint {
        EntityId id;
        bool isEnd;
        Point2D pt;
    };
    std::vector<EndPoint> endpoints;

    for (EntityId id : ids) {
        Entity* ent = m_doc.entity(id);
        if (!ent || ent->type() != EntityType::Line) continue;
        auto* line = static_cast<LineEntity*>(ent);
        endpoints.push_back({id, false, line->start()});
        endpoints.push_back({id, true,  line->end()});
    }

    std::vector<bool> handled(endpoints.size(), false);

    for (size_t i = 0; i < endpoints.size(); ++i) {
        if (handled[i]) continue;
        for (size_t j = i + 1; j < endpoints.size(); ++j) {
            if (handled[j]) continue;
            if (endpoints[i].id == endpoints[j].id) continue;

            double dist = endpoints[i].pt.distanceTo(endpoints[j].pt);
            if (dist > 0.0 && dist < gap) {
                // Move both to midpoint
                Point2D mid{
                    (endpoints[i].pt.x + endpoints[j].pt.x) * 0.5,
                    (endpoints[i].pt.y + endpoints[j].pt.y) * 0.5
                };

                auto* lineI = static_cast<LineEntity*>(m_doc.entity(endpoints[i].id));
                auto* lineJ = static_cast<LineEntity*>(m_doc.entity(endpoints[j].id));

                if (lineI) {
                    if (!endpoints[i].isEnd) lineI->setStart(mid);
                    else                     lineI->setEnd(mid);
                }
                if (lineJ) {
                    if (!endpoints[j].isEnd) lineJ->setStart(mid);
                    else                     lineJ->setEnd(mid);
                }

                // Update cached point to prevent double-snap
                endpoints[i].pt = mid;
                endpoints[j].pt = mid;

                handled[j] = true;
                ++snapped;
                break;
            }
        }
    }

    return snapped;
}

// ── totalLength ──

double Optimizer::totalLength(const std::vector<EntityId>& ids) {
    double total = 0.0;
    for (EntityId id : ids) {
        Entity* ent = m_doc.entity(id);
        if (!ent) continue;

        if (ent->type() == EntityType::Line) {
            total += static_cast<LineEntity*>(ent)->length();
        } else if (ent->type() == EntityType::Polyline) {
            auto* poly = static_cast<PolylineEntity*>(ent);
            const auto& verts = poly->vertices();
            for (size_t i = 0; i + 1 < verts.size(); ++i) {
                total += verts[i].distanceTo(verts[i + 1]);
            }
            if (poly->isClosed() && verts.size() >= 2) {
                total += verts.back().distanceTo(verts.front());
            }
        } else if (ent->type() == EntityType::Circle) {
            auto* circle = static_cast<CircleEntity*>(ent);
            total += 2.0 * PI * circle->radius();
        } else if (ent->type() == EntityType::Arc) {
            auto* arc = static_cast<ArcEntity*>(ent);
            double span = arc->endAngle() - arc->startAngle();
            if (span <= 0.0) span += 360.0;
            total += arc->radius() * (span * PI / 180.0);
        }
    }
    return total;
}

// ── lengthByClass ──

std::map<LineClassification, double> Optimizer::lengthByClass(const std::vector<EntityId>& ids) {
    std::map<LineClassification, double> result;

    for (EntityId id : ids) {
        Entity* ent = m_doc.entity(id);
        if (!ent) continue;

        LineClassification cls = ent->classification();
        double len = 0.0;

        if (ent->type() == EntityType::Line) {
            len = static_cast<LineEntity*>(ent)->length();
        } else if (ent->type() == EntityType::Polyline) {
            auto* poly = static_cast<PolylineEntity*>(ent);
            const auto& verts = poly->vertices();
            for (size_t i = 0; i + 1 < verts.size(); ++i) {
                len += verts[i].distanceTo(verts[i + 1]);
            }
            if (poly->isClosed() && verts.size() >= 2) {
                len += verts.back().distanceTo(verts.front());
            }
        } else if (ent->type() == EntityType::Circle) {
            len = 2.0 * PI * static_cast<CircleEntity*>(ent)->radius();
        } else if (ent->type() == EntityType::Arc) {
            auto* arc = static_cast<ArcEntity*>(ent);
            double span = arc->endAngle() - arc->startAngle();
            if (span <= 0.0) span += 360.0;
            len = arc->radius() * (span * PI / 180.0);
        }

        result[cls] += len;
    }

    return result;
}

} // namespace yjcad
