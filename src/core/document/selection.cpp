#include "selection.h"

namespace yjcad {

void Selection::select(EntityId id) {
    m_selected.insert(id);
}

void Selection::deselect(EntityId id) {
    m_selected.erase(id);
}

void Selection::toggle(EntityId id) {
    if (m_selected.count(id)) {
        m_selected.erase(id);
    } else {
        m_selected.insert(id);
    }
}

void Selection::clear() {
    m_selected.clear();
}

bool Selection::isSelected(EntityId id) const {
    return m_selected.count(id) > 0;
}

} // namespace yjcad
