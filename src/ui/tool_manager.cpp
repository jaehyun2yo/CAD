#include "tool_manager.h"
#include "tools/move_tool.h"
#include "tools/copy_rotate_tool.h"
#include "tools/draw_arc_tool.h"
#include "tools/mirror_tool.h"
#include "tools/scale_tool.h"
#include "tools/offset_tool.h"
#include "tools/trim_tool.h"
// Batch 3: Drawing
#include "tools/draw_rect_tool.h"
#include "tools/draw_ellipse_tool.h"
#include "tools/draw_polygon_tool.h"
#include "tools/draw_point_tool.h"
#include "tools/draw_text_tool.h"
#include "tools/draw_xline_tool.h"
// Batch 4: Editing
#include "tools/array_tool.h"
#include "tools/polar_array_tool.h"
#include "tools/stretch_tool.h"
#include "tools/extend_tool.h"
#include "tools/break_tool.h"
#include "tools/divide_tool.h"
#include "tools/chamfer_tool.h"
#include "tools/fillet_tool.h"
// Batch 5: Group
#include "tools/join_tool.h"
#include "tools/explode_tool.h"
// Batch 6: Inquiry
#include "tools/distance_tool.h"
#include "tools/area_tool.h"
// Batch 8: Dimension
#include "tools/dim_tool.h"
// Batch 10: Zoom
#include "tools/zoom_window_tool.h"

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
    reg(ToolMode::Copy, std::make_unique<CopyTool>());
    reg(ToolMode::Rotate, std::make_unique<RotateTool>());
    reg(ToolMode::DrawPolyline, std::make_unique<DrawPolylineTool>());
    reg(ToolMode::DrawArc, std::make_unique<DrawArcTool>());
    reg(ToolMode::Mirror, std::make_unique<MirrorTool>());
    reg(ToolMode::Scale, std::make_unique<ScaleTool>());
    reg(ToolMode::Offset, std::make_unique<OffsetTool>());
    reg(ToolMode::Trim, std::make_unique<TrimTool>());
    // Batch 3: Drawing
    reg(ToolMode::DrawRect, std::make_unique<DrawRectTool>());
    reg(ToolMode::DrawEllipse, std::make_unique<DrawEllipseTool>());
    reg(ToolMode::DrawPolygon, std::make_unique<DrawPolygonTool>());
    reg(ToolMode::DrawPoint, std::make_unique<DrawPointTool>());
    reg(ToolMode::DrawText, std::make_unique<DrawTextTool>());
    reg(ToolMode::DrawXLine, std::make_unique<DrawXLineTool>());
    // Batch 4: Editing
    reg(ToolMode::Array, std::make_unique<ArrayTool>());
    reg(ToolMode::PolarArray, std::make_unique<PolarArrayTool>());
    reg(ToolMode::Stretch, std::make_unique<StretchTool>());
    reg(ToolMode::Extend, std::make_unique<ExtendTool>());
    reg(ToolMode::Break, std::make_unique<BreakTool>());
    reg(ToolMode::Divide, std::make_unique<DivideTool>());
    reg(ToolMode::Chamfer, std::make_unique<ChamferTool>());
    reg(ToolMode::Fillet, std::make_unique<FilletTool>());
    // Batch 5: Group
    reg(ToolMode::Join, std::make_unique<JoinTool>());
    reg(ToolMode::Explode, std::make_unique<ExplodeTool>());
    // Batch 6: Inquiry
    reg(ToolMode::Distance, std::make_unique<DistanceTool>());
    reg(ToolMode::AreaCalc, std::make_unique<AreaTool>());
    // Batch 8: Dimension
    reg(ToolMode::Dim, std::make_unique<DimLinearTool>());
    // Batch 10: Zoom
    reg(ToolMode::ZoomWindow, std::make_unique<ZoomWindowTool>());
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
