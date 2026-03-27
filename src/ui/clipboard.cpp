#include "clipboard.h"
#include "core/document/document.h"
#include <algorithm>
#include <limits>

namespace yjcad {

// ── CadClipboard ──

void CadClipboard::copy(const Document& doc) {
    m_entities.clear();

    const auto& sel = doc.selection().selectedIds();
    if (sel.empty()) return;

    for (EntityId id : sel) {
        const Entity* ent = doc.entity(id);
        if (!ent) continue;
        m_entities.push_back(ent->clone());
    }

    m_basePoint = computeCentroid();
}

void CadClipboard::paste(Document& doc, const Point2D& basePoint) {
    if (m_entities.empty()) return;

    Point2D delta{basePoint.x - m_basePoint.x, basePoint.y - m_basePoint.y};

    for (const auto& srcEnt : m_entities) {
        // Clone again so multiple pastes work correctly
        auto clone = srcEnt->clone();
        clone->translate(delta);

        // Assign a fresh ID from the target document
        // Entity ID is immutable after construction, so we must re-clone with new ID.
        // Work around: AddEntityCommand re-assigns nothing; the entity keeps its ID.
        // To avoid ID collisions we create a deep copy via clone() and then swap the id
        // by building the concrete entity type fresh.

        EntityId newId = doc.nextId();

        std::unique_ptr<Entity> newEnt;

        if (clone->type() == EntityType::Line) {
            auto* src = static_cast<LineEntity*>(clone.get());
            newEnt = std::make_unique<LineEntity>(newId, src->start(), src->end());
        } else if (clone->type() == EntityType::Circle) {
            auto* src = static_cast<CircleEntity*>(clone.get());
            newEnt = std::make_unique<CircleEntity>(newId, src->center(), src->radius());
        } else if (clone->type() == EntityType::Arc) {
            auto* src = static_cast<ArcEntity*>(clone.get());
            newEnt = std::make_unique<ArcEntity>(newId, src->center(), src->radius(),
                                                  src->startAngle(), src->endAngle());
        } else if (clone->type() == EntityType::Polyline) {
            auto* src = static_cast<PolylineEntity*>(clone.get());
            newEnt = std::make_unique<PolylineEntity>(newId, src->vertices(), src->isClosed());
        } else if (clone->type() == EntityType::Text) {
            auto* src = static_cast<TextEntity*>(clone.get());
            newEnt = std::make_unique<TextEntity>(newId, src->position(), src->text(), src->height());
            static_cast<TextEntity*>(newEnt.get())->setRotation(src->rotation());
        } else {
            // Unsupported type — skip
            continue;
        }

        // Copy common properties
        newEnt->setLayerName(clone->layerName());
        newEnt->setColor(clone->color());
        newEnt->setLineWidth(clone->lineWidth());
        newEnt->setClassification(clone->classification());

        auto cmd = std::make_unique<AddEntityCommand>(doc, std::move(newEnt));
        doc.history().execute(std::move(cmd));
    }
}

void CadClipboard::clear() {
    m_entities.clear();
    m_basePoint = {};
}

Point2D CadClipboard::computeCentroid() const {
    if (m_entities.empty()) return {};

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    for (const auto& ent : m_entities) {
        BoundingBox bb = ent->boundingBox();
        if (!bb.isValid()) continue;
        minX = std::min(minX, bb.min.x);
        minY = std::min(minY, bb.min.y);
        maxX = std::max(maxX, bb.max.x);
        maxY = std::max(maxY, bb.max.y);
    }

    return {(minX + maxX) * 0.5, (minY + maxY) * 0.5};
}

} // namespace yjcad
