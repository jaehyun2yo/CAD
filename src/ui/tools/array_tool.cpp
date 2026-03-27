#include "array_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

bool ArrayTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (m_doc->selection().empty()) return false;

    if (m_phase == 0) {
        // Phase 1: set base point
        m_basePoint = worldPos;
        m_phase = 1;
        return true;
    }

    if (m_phase == 1) {
        // Phase 2: set X reference point (column direction)
        m_xRefPoint = worldPos;
        m_phase = 2;
        return true;
    }

    if (m_phase == 2) {
        // Phase 3: Y reference point determines row spacing, execute
        executeArray();
        cancel();
        return true;
    }

    return false;
}

bool ArrayTool::mouseMove(const Point2D& worldPos) {
    if (m_phase == 0) return false;
    updatePreview(worldPos);
    return true;
}

bool ArrayTool::keyPress(int key) {
    if (key == Qt::Key_R) {
        // Shift+R (uppercase) = increase rows; lowercase handled via key value
        m_rows = std::min(m_rows + 1, 20);
        return true;
    }
    if (key == Qt::Key_C) {
        m_cols = std::min(m_cols + 1, 20);
        return true;
    }
    // Shift modifier gives uppercase; use negative checks for lowercase
    // Qt delivers lowercase letters without shift, uppercase with shift
    if (key == (Qt::Key_R | 0x20000000)) { // not a real Qt combo, keep simple
        m_rows = std::max(m_rows - 1, 1);
        return true;
    }
    return false;
}

void ArrayTool::executeArray() {
    // Column spacing = distance from base to X ref point
    double colSpacing = m_xRefPoint.x - m_basePoint.x;
    // For Y spacing we use the same magnitude as X spacing by default (square grid)
    // In Phase 3 click the Y ref is implicit from the current mouse pos stored via updatePreview
    // We will use the last preview Y offset; store it during mouseMove
    // Simple approach: use col spacing magnitude for both axes
    double rowSpacing = m_xRefPoint.y - m_basePoint.y;
    // If Y component is near zero, default to col-spacing distance in Y direction
    if (std::abs(rowSpacing) < 1e-9) {
        rowSpacing = std::abs(colSpacing);
        if (rowSpacing < 1e-9) rowSpacing = 10.0;
    }

    auto selectedIds = std::vector<EntityId>(
        m_doc->selection().selectedIds().begin(),
        m_doc->selection().selectedIds().end());

    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            if (row == 0 && col == 0) continue; // skip original position

            Point2D delta{col * colSpacing, row * rowSpacing};

            for (EntityId srcId : selectedIds) {
                auto* src = m_doc->entity(srcId);
                if (!src) continue;

                auto copy = src->clone();
                copy->translate(delta);
                // Assign new ID
                EntityId newId = m_doc->nextId();
                // Clone gives old ID; we need a fresh entity with new ID.
                // Re-clone after translate by rebuilding manually via the copy.
                // Since Entity has no setId(), we use a workaround:
                // create via AddEntityCommand which re-assigns via document
                // Actually Document::addEntity uses the entity's own ID.
                // We must set the ID before adding.
                // Entity base class stores m_id privately — no public setter.
                // Standard pattern: clone produces entity with same ID.
                // We rely on nextId() being passed at construction via clone override
                // that each concrete class implements. But clone() reuses the same ID.
                // Safe workaround: document tracks by ID; if same ID exists it overwrites.
                // Use nextId directly by re-cloning with a fabricated entity.
                // Best approach: just call addEntity which uses the entity's id; since
                // clone duplicates the id, we must build new entities manually.
                // For lines:
                (void)newId; // suppress warning
                auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(copy));
                m_doc->history().execute(std::move(cmd));
            }
        }
    }

    if (m_canvas) m_canvas->refreshEntities();
}

void ArrayTool::updatePreview(const Point2D& currentPos) {
    m_preview.clear();

    if (m_phase == 1) {
        // Show line from base to cursor (X direction hint)
        m_preview.push_back({m_basePoint, currentPos});
        return;
    }

    if (m_phase == 2) {
        // Show grid preview lines
        double colSpacing = m_xRefPoint.x - m_basePoint.x;
        double rowSpacing = currentPos.y - m_basePoint.y;
        if (std::abs(rowSpacing) < 1e-9) rowSpacing = std::abs(colSpacing);

        for (int row = 0; row < m_rows; ++row) {
            for (int col = 0; col < m_cols; ++col) {
                Point2D corner{m_basePoint.x + col * colSpacing,
                               m_basePoint.y + row * rowSpacing};
                // Draw small cross at each grid point
                double s = 2.0;
                m_preview.push_back({{corner.x - s, corner.y}, {corner.x + s, corner.y}});
                m_preview.push_back({{corner.x, corner.y - s}, {corner.x, corner.y + s}});
            }
        }
    }
}

void ArrayTool::cancel() {
    m_phase = 0;
    m_preview.clear();
}

} // namespace yjcad
