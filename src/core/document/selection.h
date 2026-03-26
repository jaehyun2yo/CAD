#pragma once

#include "entity.h"
#include <unordered_set>
#include <vector>

namespace yjcad {

/// Selection manager — tracks selected entities
class Selection {
public:
    /// Select an entity (adds to selection)
    void select(EntityId id);

    /// Deselect an entity
    void deselect(EntityId id);

    /// Toggle selection
    void toggle(EntityId id);

    /// Clear all selection
    void clear();

    /// Check if entity is selected
    bool isSelected(EntityId id) const;

    /// Get all selected entity IDs
    const std::unordered_set<EntityId>& selectedIds() const { return m_selected; }

    /// Number of selected entities
    size_t count() const { return m_selected.size(); }

    bool empty() const { return m_selected.empty(); }

private:
    std::unordered_set<EntityId> m_selected;
};

} // namespace yjcad
