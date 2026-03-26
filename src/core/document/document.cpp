#include "document.h"
#include <limits>

namespace yjcad {

Document::Document() = default;

EntityId Document::addEntity(std::unique_ptr<Entity> entity) {
    EntityId id = entity->id();
    m_entities[id] = std::move(entity);
    m_modified = true;
    return id;
}

std::unique_ptr<Entity> Document::removeEntity(EntityId id) {
    auto it = m_entities.find(id);
    if (it == m_entities.end()) return nullptr;
    auto entity = std::move(it->second);
    m_entities.erase(it);
    m_selection.deselect(id);
    m_modified = true;
    return entity;
}

Entity* Document::entity(EntityId id) {
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

const Entity* Document::entity(EntityId id) const {
    auto it = m_entities.find(id);
    return it != m_entities.end() ? it->second.get() : nullptr;
}

std::vector<EntityId> Document::entityIds() const {
    std::vector<EntityId> ids;
    ids.reserve(m_entities.size());
    for (const auto& [id, _] : m_entities) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<EntityId> Document::entitiesInBox(const BoundingBox& box) const {
    std::vector<EntityId> result;
    // Linear scan — TODO: replace with R-tree spatial index for performance
    for (const auto& [id, entity] : m_entities) {
        if (entity->isVisible() && entity->boundingBox().intersects(box)) {
            result.push_back(id);
        }
    }
    return result;
}

BoundingBox Document::bounds() const {
    if (m_entities.empty()) return {{0, 0}, {0, 0}};
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();
    for (const auto& [_, entity] : m_entities) {
        auto bb = entity->boundingBox();
        minX = std::min(minX, bb.min.x);
        minY = std::min(minY, bb.min.y);
        maxX = std::max(maxX, bb.max.x);
        maxY = std::max(maxY, bb.max.y);
    }
    return {{minX, minY}, {maxX, maxY}};
}

void Document::clear() {
    m_entities.clear();
    m_layers = LayerManager();
    m_selection.clear();
    m_history.clear();
    m_nextId = 1;
    m_modified = false;
    m_filePath.clear();
}

// ── AddEntityCommand ──

AddEntityCommand::AddEntityCommand(Document& doc, std::unique_ptr<Entity> entity)
    : m_doc(doc), m_entity(std::move(entity)), m_id(m_entity->id()) {}

void AddEntityCommand::execute() {
    if (m_executed) {
        // Re-execute after undo — entity was stored back
        m_doc.addEntity(std::move(m_entity));
    } else {
        m_doc.addEntity(std::move(m_entity));
        m_executed = true;
    }
}

void AddEntityCommand::undo() {
    m_entity = m_doc.removeEntity(m_id);
}

// ── RemoveEntityCommand ──

RemoveEntityCommand::RemoveEntityCommand(Document& doc, EntityId id)
    : m_doc(doc), m_id(id) {}

void RemoveEntityCommand::execute() {
    m_entity = m_doc.removeEntity(m_id);
}

void RemoveEntityCommand::undo() {
    if (m_entity) {
        m_doc.addEntity(std::move(m_entity));
    }
}

// ── MoveCommand ──

MoveCommand::MoveCommand(Document& doc, const std::vector<EntityId>& ids, const Point2D& delta)
    : m_doc(doc), m_ids(ids), m_delta(delta) {}

void MoveCommand::execute() {
    for (EntityId id : m_ids) {
        if (auto* e = m_doc.entity(id)) {
            e->translate(m_delta);
        }
    }
    m_doc.setModified(true);
}

void MoveCommand::undo() {
    Point2D reverseDelta = {-m_delta.x, -m_delta.y};
    for (EntityId id : m_ids) {
        if (auto* e = m_doc.entity(id)) {
            e->translate(reverseDelta);
        }
    }
}

} // namespace yjcad
