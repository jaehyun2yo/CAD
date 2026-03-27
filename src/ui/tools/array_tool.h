#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Array tool — copies selected entities into a rectangular N×M grid
/// Phase 1: Click base point
/// Phase 2: Click X-direction reference (sets column spacing)
/// Phase 3: Click Y-direction reference (sets row spacing, executes)
/// Keys: R = increase rows, r = decrease rows, C = increase cols, c = decrease cols
class ArrayTool : public Tool {
public:
    std::string name() const override { return "배열"; }
    ToolMode mode() const override { return ToolMode::Array; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    bool keyPress(int key) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void executeArray();
    void updatePreview(const Point2D& currentPos);

    int m_phase = 0; // 0=waiting base, 1=waiting X ref, 2=waiting Y ref
    Point2D m_basePoint;
    Point2D m_xRefPoint;
    int m_rows = 2;
    int m_cols = 2;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
