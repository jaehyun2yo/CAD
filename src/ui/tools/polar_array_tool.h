#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Polar Array tool — copies selected entities around a center in a full 360° sweep
/// Phase 1: Click center point
/// Phase 2: Click reference point → executes with current count
/// Key N / n: increase / decrease copy count (default 6)
class PolarArrayTool : public Tool {
public:
    std::string name() const override { return "원형 배열"; }
    ToolMode mode() const override { return ToolMode::PolarArray; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    bool keyPress(int key) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void executePolarArray(const Point2D& refPoint);
    void updatePreview(const Point2D& currentPos);

    bool m_hasCenter = false;
    Point2D m_center;
    int m_count = 6; // number of copies (including original)
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
