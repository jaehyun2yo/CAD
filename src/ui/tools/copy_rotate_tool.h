#pragma once

#include "ui/tool.h"

namespace yjcad {

/// Copy tool — copies selected entities with offset
class CopyTool : public Tool {
public:
    std::string name() const override { return "복사"; }
    ToolMode mode() const override { return ToolMode::Copy; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    bool m_hasBase = false;
    Point2D m_basePoint;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

/// Rotate tool — rotates selected entities around a base point
class RotateTool : public Tool {
public:
    std::string name() const override { return "회전"; }
    ToolMode mode() const override { return ToolMode::Rotate; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    int m_clickCount = 0;
    Point2D m_center;
    Point2D m_refPoint;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
