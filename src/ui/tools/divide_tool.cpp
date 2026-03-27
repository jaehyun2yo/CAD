#include "divide_tool.h"
#include "core/document/document.h"
#include "core/geometry/operations.h"
#include "ui/cad_canvas.h"
#include <cmath>

namespace yjcad {

static constexpr double kMarkerHalfSize = 1.5; // mm — cross marker half-size

bool DivideTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (!m_lineSelected) {
        // Pick closest line
        EntityId closestId = 0;
        double closestDist = 1e18;

        for (EntityId id : m_doc->entityIds()) {
            auto* entity = m_doc->entity(id);
            if (!entity || !entity->isVisible()) continue;
            if (entity->type() != EntityType::Line) continue;

            auto* line = static_cast<LineEntity*>(entity);
            double d = GeometryOps::pointToSegmentDistance(
                worldPos, line->start(), line->end());
            if (d < closestDist) {
                closestDist = d;
                closestId = id;
            }
        }

        if (closestId == 0) return false;

        m_targetId = closestId;
        m_lineSelected = true;
        buildPreview();
        return true;
    }

    return false;
}

bool DivideTool::mouseMove(const Point2D& worldPos) {
    (void)worldPos;
    return false;
}

bool DivideTool::keyPress(int key) {
    if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
        m_divisions = std::min(m_divisions + 1, 100);
        buildPreview();
        return true;
    }
    if (key == Qt::Key_Minus) {
        m_divisions = std::max(m_divisions - 1, 2);
        buildPreview();
        return true;
    }
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (m_lineSelected) {
            executeDivide();
            cancel();
            return true;
        }
    }
    return false;
}

void DivideTool::executeDivide() {
    auto* entity = m_doc->entity(m_targetId);
    if (!entity || entity->type() != EntityType::Line) return;

    auto* line = static_cast<LineEntity*>(entity);
    double dx = line->end().x - line->start().x;
    double dy = line->end().y - line->start().y;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-12) return;

    std::string layerName = line->layerName();
    int color             = line->color();

    for (int i = 1; i < m_divisions; ++i) {
        double t = static_cast<double>(i) / m_divisions;
        Point2D pt{line->start().x + t * dx, line->start().y + t * dy};

        double s = kMarkerHalfSize;
        // Horizontal arm
        auto hLine = std::make_unique<LineEntity>(
            m_doc->nextId(),
            Point2D{pt.x - s, pt.y},
            Point2D{pt.x + s, pt.y});
        hLine->setLayerName(layerName);
        hLine->setColor(color);
        auto addH = std::make_unique<AddEntityCommand>(*m_doc, std::move(hLine));
        m_doc->history().execute(std::move(addH));

        // Vertical arm
        auto vLine = std::make_unique<LineEntity>(
            m_doc->nextId(),
            Point2D{pt.x, pt.y - s},
            Point2D{pt.x, pt.y + s});
        vLine->setLayerName(layerName);
        vLine->setColor(color);
        auto addV = std::make_unique<AddEntityCommand>(*m_doc, std::move(vLine));
        m_doc->history().execute(std::move(addV));
    }

    if (m_canvas) m_canvas->refreshEntities();
}

void DivideTool::buildPreview() {
    m_preview.clear();
    if (!m_lineSelected) return;

    auto* entity = m_doc->entity(m_targetId);
    if (!entity || entity->type() != EntityType::Line) return;

    auto* line = static_cast<LineEntity*>(entity);
    double dx = line->end().x - line->start().x;
    double dy = line->end().y - line->start().y;

    // Show line and marker positions
    m_preview.push_back({line->start(), line->end()});

    for (int i = 1; i < m_divisions; ++i) {
        double t = static_cast<double>(i) / m_divisions;
        Point2D pt{line->start().x + t * dx, line->start().y + t * dy};
        double s = kMarkerHalfSize;
        m_preview.push_back({{pt.x - s, pt.y}, {pt.x + s, pt.y}});
        m_preview.push_back({{pt.x, pt.y - s}, {pt.x, pt.y + s}});
    }
}

void DivideTool::cancel() {
    m_lineSelected = false;
    m_targetId = 0;
    m_preview.clear();
}

} // namespace yjcad
