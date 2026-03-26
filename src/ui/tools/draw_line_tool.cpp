#include "draw_line_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;

// ── DrawLineTool ──

bool DrawLineTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_hasFirst) {
        m_firstPoint = worldPos;
        m_hasFirst = true;
        return true;
    }

    // Second click — create line via command
    EntityId id = m_doc->nextId();
    auto line = std::make_unique<LineEntity>(id, m_firstPoint, worldPos);
    line->setLayerName(m_doc->layers().currentLayer());
    auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(line));
    m_doc->history().execute(std::move(cmd));

    // Continue drawing from last point (chain mode)
    m_firstPoint = worldPos;
    m_preview.clear();

    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool DrawLineTool::mouseMove(const Point2D& worldPos) {
    m_currentPos = worldPos;
    m_preview.clear();
    if (m_hasFirst) {
        m_preview.push_back({m_firstPoint, worldPos});
    }
    return m_hasFirst;
}

void DrawLineTool::cancel() {
    m_hasFirst = false;
    m_preview.clear();
}

// ── DrawCircleTool ──

bool DrawCircleTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_hasCenter) {
        m_center = worldPos;
        m_hasCenter = true;
        return true;
    }

    // Second click — create circle
    double radius = m_center.distanceTo(worldPos);
    if (radius < 1e-9) return true;

    EntityId id = m_doc->nextId();
    auto circle = std::make_unique<CircleEntity>(id, m_center, radius);
    circle->setLayerName(m_doc->layers().currentLayer());
    auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(circle));
    m_doc->history().execute(std::move(cmd));

    m_hasCenter = false;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool DrawCircleTool::mouseMove(const Point2D& worldPos) {
    m_currentPos = worldPos;
    m_preview.clear();
    if (m_hasCenter) {
        double radius = m_center.distanceTo(worldPos);
        buildCirclePreview(m_center, radius);
    }
    return m_hasCenter;
}

void DrawCircleTool::cancel() {
    m_hasCenter = false;
    m_preview.clear();
}

void DrawCircleTool::buildCirclePreview(const Point2D& center, double radius) {
    constexpr int SEGS = 48;
    for (int i = 0; i < SEGS; ++i) {
        double a1 = 2.0 * PI * i / SEGS;
        double a2 = 2.0 * PI * (i + 1) / SEGS;
        Point2D p1{center.x + radius * std::cos(a1), center.y + radius * std::sin(a1)};
        Point2D p2{center.x + radius * std::cos(a2), center.y + radius * std::sin(a2)};
        m_preview.push_back({p1, p2});
    }
}

// ── DeleteTool ──

bool DeleteTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button != Qt::LeftButton) return false;
    (void)worldPos; // hit test is done in CadCanvas

    // Delete all selected entities
    auto selectedIds = m_doc->selection().selectedIds();
    for (EntityId id : selectedIds) {
        auto cmd = std::make_unique<RemoveEntityCommand>(*m_doc, id);
        m_doc->history().execute(std::move(cmd));
    }
    m_doc->selection().clear();
    if (m_canvas) m_canvas->refreshEntities();
    return !selectedIds.empty();
}

} // namespace yjcad
