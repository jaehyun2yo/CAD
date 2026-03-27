#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Trim tool — trims a line at intersection with boundary entities
/// Phase 1: Select boundary entities (already selected)
/// Phase 2: Click on segment to trim (removes the clicked portion)
class TrimTool : public Tool {
public:
    std::string name() const override { return "트림"; }
    ToolMode mode() const override { return ToolMode::Trim; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    // Boundary entity IDs (selected before tool activation)
    std::vector<EntityId> m_boundaryIds;
    bool m_boundariesSet = false;
    std::vector<std::pair<Point2D, Point2D>> m_preview;

    // Find all intersection points between a line and boundary entities
    struct IntersectionInfo {
        Point2D point;
        double t; // parameter along the line (0=start, 1=end)
    };
    std::vector<IntersectionInfo> findIntersections(const LineEntity& line);
};

} // namespace yjcad
