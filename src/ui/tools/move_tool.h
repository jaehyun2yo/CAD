#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Move tool — click base point, click destination, move selected entities
class MoveTool : public Tool {
public:
    std::string name() const override { return "이동"; }
    ToolMode mode() const override { return ToolMode::Move; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    bool m_hasBase = false;
    Point2D m_basePoint;
    Point2D m_currentPos;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

/// Polyline draw tool — click vertices, right-click or Enter to finish
class DrawPolylineTool : public Tool {
public:
    std::string name() const override { return "폴리라인 그리기"; }
    ToolMode mode() const override { return ToolMode::DrawPolyline; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    bool keyPress(int key) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    void finishPolyline(bool closed);
    std::vector<Point2D> m_vertices;
    Point2D m_currentPos;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

} // namespace yjcad
