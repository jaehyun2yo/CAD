#include "tool_manager.h"
#include "tools/move_tool.h"

namespace yjcad {

ToolManager::ToolManager(Document& doc, CadCanvas* canvas)
    : m_doc(doc), m_canvas(canvas) {
    // Register all tools
    auto reg = [&](ToolMode mode, std::unique_ptr<Tool> tool) {
        tool->setDocument(&m_doc);
        tool->setCanvas(m_canvas);
        m_tools[mode] = std::move(tool);
    };

    reg(ToolMode::DrawLine, std::make_unique<DrawLineTool>());
    reg(ToolMode::DrawCircle, std::make_unique<DrawCircleTool>());
    reg(ToolMode::Delete, std::make_unique<DeleteTool>());
    reg(ToolMode::Move, std::make_unique<MoveTool>());
    reg(ToolMode::DrawPolyline, std::make_unique<DrawPolylineTool>());
    // Select mode has no dedicated tool — handled by CadCanvas directly
}

void ToolManager::setTool(ToolMode mode) {
    if (m_current) m_current->cancel();
    m_currentMode = mode;
    auto it = m_tools.find(mode);
    m_current = (it != m_tools.end()) ? it->second.get() : nullptr;
}

void ToolManager::cancelToSelect() {
    if (m_current) m_current->cancel();
    m_currentMode = ToolMode::Select;
    m_current = nullptr;
}

bool ToolManager::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (m_current) return m_current->mousePress(worldPos, button);
    return false;
}

bool ToolManager::mouseMove(const Point2D& worldPos) {
    if (m_current) return m_current->mouseMove(worldPos);
    return false;
}

bool ToolManager::mouseRelease(const Point2D& worldPos, Qt::MouseButton button) {
    if (m_current) return m_current->mouseRelease(worldPos, button);
    return false;
}

bool ToolManager::keyPress(int key) {
    if (key == Qt::Key_Escape) {
        cancelToSelect();
        return true;
    }
    if (m_current) return m_current->keyPress(key);
    return false;
}

} // namespace yjcad
