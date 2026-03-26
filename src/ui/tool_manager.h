#pragma once

#include "tool.h"
#include "tools/draw_line_tool.h"
#include <memory>
#include <unordered_map>
#include <functional>

namespace yjcad {

/// Manages active tool and tool switching
class ToolManager {
public:
    explicit ToolManager(Document& doc, CadCanvas* canvas);

    /// Switch to a specific tool mode
    void setTool(ToolMode mode);

    /// Get current tool
    Tool* currentTool() { return m_current; }
    ToolMode currentMode() const { return m_currentMode; }

    /// Cancel current tool and return to Select
    void cancelToSelect();

    /// Forward mouse/key events to current tool
    bool mousePress(const Point2D& worldPos, Qt::MouseButton button);
    bool mouseMove(const Point2D& worldPos);
    bool mouseRelease(const Point2D& worldPos, Qt::MouseButton button);
    bool keyPress(int key);

private:
    Document& m_doc;
    CadCanvas* m_canvas;

    std::unordered_map<ToolMode, std::unique_ptr<Tool>> m_tools;
    Tool* m_current = nullptr;
    ToolMode m_currentMode = ToolMode::Select;
};

} // namespace yjcad
