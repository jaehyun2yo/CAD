#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Join tool — finds connected LineEntity endpoints and merges them into a PolylineEntity
class JoinTool : public Tool {
public:
    std::string name() const override { return "결합"; }
    ToolMode mode() const override { return ToolMode::Join; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    /// Execute join on selected entities
    void executeJoin();

    static constexpr double kEpsilon = 0.01; // mm

    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
