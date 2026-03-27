#include "mirror_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"

namespace yjcad {

bool MirrorTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;
    if (m_doc->selection().empty()) return false;

    if (!m_hasFirst) {
        m_axisStart = worldPos;
        m_hasFirst = true;
        return true;
    }

    // Second click — execute mirror
    auto ids = std::vector<EntityId>(
        m_doc->selection().selectedIds().begin(),
        m_doc->selection().selectedIds().end());

    auto cmd = std::make_unique<MirrorCommand>(*m_doc, ids, m_axisStart, worldPos);
    m_doc->history().execute(std::move(cmd));

    m_hasFirst = false;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool MirrorTool::mouseMove(const Point2D& worldPos) {
    m_preview.clear();
    if (m_hasFirst) {
        m_preview.push_back({m_axisStart, worldPos});
    }
    return m_hasFirst;
}

void MirrorTool::cancel() {
    m_hasFirst = false;
    m_preview.clear();
}

} // namespace yjcad
