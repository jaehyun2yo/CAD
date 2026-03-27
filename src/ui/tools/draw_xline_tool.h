#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Interactive construction line (XLine) drawing tool
/// 2-click: origin point → direction point → very long LineEntity (±10000mm extension)
/// Entity is placed on the "구아이" layer with Guide classification
class DrawXLineTool : public Tool {
public:
    std::string name() const override { return "구성선"; }
    ToolMode mode() const override { return ToolMode::DrawXLine; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    bool m_hasOrigin = false;
    Point2D m_origin;
    Point2D m_currentPos;
    std::vector<std::pair<Point2D, Point2D>> m_preview;

    static constexpr double EXTEND_LENGTH = 10000.0; // mm
    static constexpr const char* GUIDE_LAYER = "구아이";

    std::pair<Point2D, Point2D> buildXLine(const Point2D& origin,
                                            const Point2D& directionPt) const;
};

} // namespace yjcad
