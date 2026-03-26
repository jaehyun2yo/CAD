#pragma once

#include "ui/tool.h"
#include <vector>

namespace yjcad {

/// Interactive line drawing tool
/// Click first point → rubber-band preview → click second point → create line
class DrawLineTool : public Tool {
public:
    std::string name() const override { return "선 그리기"; }
    ToolMode mode() const override { return ToolMode::DrawLine; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    bool m_hasFirst = false;
    Point2D m_firstPoint;
    Point2D m_currentPos;
    std::vector<std::pair<Point2D, Point2D>> m_preview;
};

/// Interactive circle drawing tool
/// Click center → drag for radius → release to create
class DrawCircleTool : public Tool {
public:
    std::string name() const override { return "원 그리기"; }
    ToolMode mode() const override { return ToolMode::DrawCircle; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override;
    void cancel() override;

    const std::vector<std::pair<Point2D, Point2D>>& previewLines() const override {
        return m_preview;
    }

private:
    bool m_hasCenter = false;
    Point2D m_center;
    Point2D m_currentPos;
    std::vector<std::pair<Point2D, Point2D>> m_preview;

    void buildCirclePreview(const Point2D& center, double radius);
};

/// Delete tool — click to delete entity
class DeleteTool : public Tool {
public:
    std::string name() const override { return "삭제"; }
    ToolMode mode() const override { return ToolMode::Delete; }

    bool mousePress(const Point2D& worldPos, Qt::MouseButton button) override;
    bool mouseMove(const Point2D& worldPos) override { (void)worldPos; return false; }
};

} // namespace yjcad
