#include "stretch_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <algorithm>

namespace yjcad {

bool StretchTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (m_phase == 0) {
        m_boxCorner1 = worldPos;
        m_phase = 1;
        return true;
    }

    if (m_phase == 1) {
        m_boxCorner2 = worldPos;
        m_phase = 2;
        return true;
    }

    if (m_phase == 2) {
        m_basePoint = worldPos;
        m_phase = 3;
        return true;
    }

    if (m_phase == 3) {
        executeStretch(worldPos);
        cancel();
        return true;
    }

    return false;
}

bool StretchTool::mouseMove(const Point2D& worldPos) {
    m_preview.clear();

    if (m_phase == 1) {
        // Show rubber-band box
        m_preview.push_back({m_boxCorner1, {worldPos.x, m_boxCorner1.y}});
        m_preview.push_back({{worldPos.x, m_boxCorner1.y}, worldPos});
        m_preview.push_back({worldPos, {m_boxCorner1.x, worldPos.y}});
        m_preview.push_back({{m_boxCorner1.x, worldPos.y}, m_boxCorner1});
        return true;
    }

    if (m_phase == 2) {
        // Show fixed box + line from where base will be
        double xMin = std::min(m_boxCorner1.x, m_boxCorner2.x);
        double xMax = std::max(m_boxCorner1.x, m_boxCorner2.x);
        double yMin = std::min(m_boxCorner1.y, m_boxCorner2.y);
        double yMax = std::max(m_boxCorner1.y, m_boxCorner2.y);
        m_preview.push_back({{xMin, yMin}, {xMax, yMin}});
        m_preview.push_back({{xMax, yMin}, {xMax, yMax}});
        m_preview.push_back({{xMax, yMax}, {xMin, yMax}});
        m_preview.push_back({{xMin, yMax}, {xMin, yMin}});
        return true;
    }

    if (m_phase == 3) {
        // Show displacement vector
        m_preview.push_back({m_basePoint, worldPos});
        return true;
    }

    return false;
}

void StretchTool::executeStretch(const Point2D& destination) {
    Point2D delta{destination.x - m_basePoint.x,
                  destination.y - m_basePoint.y};

    double xMin = std::min(m_boxCorner1.x, m_boxCorner2.x);
    double xMax = std::max(m_boxCorner1.x, m_boxCorner2.x);
    double yMin = std::min(m_boxCorner1.y, m_boxCorner2.y);
    double yMax = std::max(m_boxCorner1.y, m_boxCorner2.y);

    BoundingBox stretchBox;
    stretchBox.min = {xMin, yMin};
    stretchBox.max = {xMax, yMax};

    auto inBox = [&](const Point2D& p) {
        return stretchBox.contains(p);
    };

    // Iterate all line entities and move endpoints that are inside the box
    for (EntityId id : m_doc->entityIds()) {
        auto* entity = m_doc->entity(id);
        if (!entity || !entity->isVisible()) continue;
        if (entity->type() != EntityType::Line) continue;

        auto* line = static_cast<LineEntity*>(entity);
        bool startIn = inBox(line->start());
        bool endIn   = inBox(line->end());

        if (!startIn && !endIn) continue;

        // Build new endpoints
        Point2D newStart = startIn
            ? Point2D{line->start().x + delta.x, line->start().y + delta.y}
            : line->start();
        Point2D newEnd = endIn
            ? Point2D{line->end().x + delta.x, line->end().y + delta.y}
            : line->end();

        // Remove old line, add new line
        EntityId newId = m_doc->nextId();
        auto rmCmd = std::make_unique<RemoveEntityCommand>(*m_doc, id);
        m_doc->history().execute(std::move(rmCmd));

        auto newLine = std::make_unique<LineEntity>(newId, newStart, newEnd);
        newLine->setLayerName(line->layerName());
        newLine->setColor(line->color());
        newLine->setClassification(line->classification());
        auto addCmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(newLine));
        m_doc->history().execute(std::move(addCmd));
    }

    if (m_canvas) m_canvas->refreshEntities();
}

void StretchTool::cancel() {
    m_phase = 0;
    m_preview.clear();
}

} // namespace yjcad
