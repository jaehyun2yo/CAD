#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Extend tool — extends a line's nearest endpoint to the first boundary it can reach
/// Phase 1: First click captures current selection as boundary entities
/// Phase 2+: Click near the end of a line to extend it to the boundary intersection
class ExtendTool : public Tool {
public:
    std::string name() const override { return "연장"; }
    ToolMode mode() const override { return ToolMode::Extend; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    struct IntersectionInfo {
        Point2D point;
        double t; // parameter along the line direction (can be > 1 or < 0 for extensions)
    };

    /// Find the closest boundary intersection in the extension direction from an endpoint
    bool findExtendTarget(const LineEntity& line, bool extendEnd,
                          Point2D& outTarget);

    std::vector<EntityId> m_boundaryIds;
    bool m_boundariesSet = false;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
