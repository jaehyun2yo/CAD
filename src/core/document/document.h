#pragma once

#include "entity.h"
#include "layer.h"
#include "selection.h"
#include "command.h"
#include <unordered_map>
#include <functional>

namespace yjcad {

/// Document — the central data model for YJ-CAD
/// Owns all entities, layers, selection, and command history.
class Document {
public:
    Document();

    // ── Entity management ──

    /// Add entity to document (takes ownership). Returns entity ID.
    EntityId addEntity(std::unique_ptr<Entity> entity);

    /// Remove entity by ID (returns removed entity, or nullptr)
    std::unique_ptr<Entity> removeEntity(EntityId id);

    /// Get entity by ID (nullptr if not found)
    Entity* entity(EntityId id);
    const Entity* entity(EntityId id) const;

    /// Get all entity IDs
    std::vector<EntityId> entityIds() const;

    /// Number of entities
    size_t entityCount() const { return m_entities.size(); }

    /// Get entities in bounding box (for viewport culling)
    std::vector<EntityId> entitiesInBox(const BoundingBox& box) const;

    /// Get bounding box of all entities
    BoundingBox bounds() const;

    // ── ID generator ──

    EntityId nextId() { return m_nextId++; }

    // ── Layer management ──

    LayerManager& layers() { return m_layers; }
    const LayerManager& layers() const { return m_layers; }

    // ── Selection ──

    Selection& selection() { return m_selection; }
    const Selection& selection() const { return m_selection; }

    // ── Command history (Undo/Redo) ──

    CommandHistory& history() { return m_history; }
    const CommandHistory& history() const { return m_history; }

    // ── Document state ──

    bool isModified() const { return m_modified; }
    void setModified(bool m) { m_modified = m; }

    const std::string& filePath() const { return m_filePath; }
    void setFilePath(const std::string& path) { m_filePath = path; }

    /// Clear all entities, layers, selection, history
    void clear();

private:
    std::unordered_map<EntityId, std::unique_ptr<Entity>> m_entities;
    LayerManager m_layers;
    Selection m_selection;
    CommandHistory m_history;
    EntityId m_nextId = 1;
    bool m_modified = false;
    std::string m_filePath;
};

// ── Concrete commands for Undo/Redo ──

/// Command: Add entity
class AddEntityCommand : public Command {
public:
    AddEntityCommand(Document& doc, std::unique_ptr<Entity> entity);
    void execute() override;
    void undo() override;
    std::string description() const override { return "Add entity"; }

private:
    Document& m_doc;
    std::unique_ptr<Entity> m_entity;
    EntityId m_id;
    bool m_executed = false;
};

/// Command: Remove entity
class RemoveEntityCommand : public Command {
public:
    RemoveEntityCommand(Document& doc, EntityId id);
    void execute() override;
    void undo() override;
    std::string description() const override { return "Remove entity"; }

private:
    Document& m_doc;
    std::unique_ptr<Entity> m_entity;
    EntityId m_id;
};

/// Command: Move entities
class MoveCommand : public Command {
public:
    MoveCommand(Document& doc, const std::vector<EntityId>& ids, const Point2D& delta);
    void execute() override;
    void undo() override;
    std::string description() const override { return "Move"; }

private:
    Document& m_doc;
    std::vector<EntityId> m_ids;
    Point2D m_delta;
};

} // namespace yjcad
