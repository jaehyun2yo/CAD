#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Interactive text drawing tool
/// 1-click: select position → QInputDialog prompts for text and height → TextEntity created
class DrawTextTool : public Tool {
public:
    std::string name() const override { return "텍스트"; }
    ToolMode mode() const override { return ToolMode::DrawText; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override { (void)worldPos; return false; }
    void cancel() override {}

    // No rubber-band preview — single-click tool
    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        static std::vector<std::pair<Point2D, Point2D>> empty;
        return empty;
    }

private:
    static constexpr double DEFAULT_HEIGHT = 5.0; // mm
};

} // namespace yjcad
