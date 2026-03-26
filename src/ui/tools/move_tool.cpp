#include "move_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"

namespace yjcad {

// ── MoveTool ──

bool MoveTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (m_doc->selection().empty()) return false;

    if (!m_hasBase) {
        m_basePoint = worldPos;
        m_hasBase = true;
        return true;
    }

    // Second click — execute move
    Point2D delta{worldPos.x - m_basePoint.x, worldPos.y - m_basePoint.y};
    auto ids = std::vector<EntityId>(
        m_doc->selection().selectedIds().begin(),
        m_doc->selection().selectedIds().end());

    auto cmd = std::make_unique<MoveCommand>(*m_doc, ids, delta);
    m_doc->history().execute(std::move(cmd));

    m_hasBase = false;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool MoveTool::mouseMove(const Point2D& worldPos) {
    m_currentPos = worldPos;
    m_preview.clear();

    if (m_hasBase) {
        // Show a guide line from base to current
        m_preview.push_back({m_basePoint, worldPos});
    }
    return m_hasBase;
}

void MoveTool::cancel() {
    m_hasBase = false;
    m_preview.clear();
}

// ── DrawPolylineTool ──

bool DrawPolylineTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) {
        if (m_vertices.size() >= 2) {
            finishPolyline(false);
        } else {
            cancel();
        }
        return true;
    }
    if (button != Qt::LeftButton) return false;

    m_vertices.push_back(worldPos);
    return true;
}

bool DrawPolylineTool::mouseMove(const Point2D& worldPos) {
    m_currentPos = worldPos;
    m_preview.clear();

    // Draw completed segments
    for (size_t i = 0; i + 1 < m_vertices.size(); ++i) {
        m_preview.push_back({m_vertices[i], m_vertices[i + 1]});
    }
    // Rubber-band to cursor
    if (!m_vertices.empty()) {
        m_preview.push_back({m_vertices.back(), worldPos});
    }
    return !m_vertices.empty();
}

bool DrawPolylineTool::keyPress(int key) {
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (m_vertices.size() >= 2) {
            finishPolyline(false);
            return true;
        }
    }
    if (key == Qt::Key_C) {
        // Close polyline
        if (m_vertices.size() >= 3) {
            finishPolyline(true);
            return true;
        }
    }
    return false;
}

void DrawPolylineTool::finishPolyline(bool closed) {
    if (m_vertices.size() < 2) return;

    EntityId id = m_doc->nextId();
    auto poly = std::make_unique<PolylineEntity>(id, m_vertices, closed);
    poly->setLayerName(m_doc->layers().currentLayer());
    auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(poly));
    m_doc->history().execute(std::move(cmd));

    m_vertices.clear();
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
}

void DrawPolylineTool::cancel() {
    m_vertices.clear();
    m_preview.clear();
}

} // namespace yjcad
