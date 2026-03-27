#include "draw_text_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <QInputDialog>
#include <QWidget>
#include <QString>

namespace yjcad {

// ── DrawTextTool ──

bool DrawTextTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) return true;
    if (button != Qt::LeftButton) return false;

    QWidget* parent = m_canvas ? dynamic_cast<QWidget*>(m_canvas) : nullptr;

    // Prompt for text content
    bool okText = false;
    QString text = QInputDialog::getText(parent,
        "텍스트 입력", "텍스트:",
        QLineEdit::Normal, "", &okText);
    if (!okText || text.isEmpty()) return true;

    // Prompt for text height
    bool okHeight = false;
    double height = QInputDialog::getDouble(parent,
        "텍스트 높이", "높이 (mm):",
        DEFAULT_HEIGHT, 0.1, 10000.0, 2, &okHeight);
    if (!okHeight) return true;

    EntityId id = m_doc->nextId();
    auto textEntity = std::make_unique<TextEntity>(
        id, worldPos, text.toStdString(), height);
    textEntity->setLayerName(m_doc->layers().currentLayer());
    auto cmd = std::make_unique<AddEntityCommand>(*m_doc, std::move(textEntity));
    m_doc->history().execute(std::move(cmd));

    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

} // namespace yjcad
