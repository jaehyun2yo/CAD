#include "ui/command_line.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QFont>

namespace yjcad {

CommandLineWidget::CommandLineWidget(QWidget* parent)
    : QWidget(parent)
{
    // ── Layout ──────────────────────────────────────────────
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(4, 2, 4, 2);
    outerLayout->setSpacing(1);

    // Message line (output from commands)
    m_messageLabel = new QLabel(this);
    m_messageLabel->setStyleSheet(
        "color: #aaaaaa; font-size: 11px; padding: 0 4px;");
    m_messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    outerLayout->addWidget(m_messageLabel);

    // Input row: [명령:] [__input field__]
    auto* inputRow = new QHBoxLayout();
    inputRow->setContentsMargins(0, 0, 0, 0);
    inputRow->setSpacing(4);

    m_promptLabel = new QLabel("명령:", this);
    m_promptLabel->setStyleSheet(
        "color: #888888; font-size: 12px; padding: 0 4px;");
    m_promptLabel->setFixedWidth(46);

    m_input = new QLineEdit(this);
    m_input->setStyleSheet(
        "QLineEdit {"
        "  background: #1e1e1e;"
        "  color: #dddddd;"
        "  border: 1px solid #444444;"
        "  border-radius: 2px;"
        "  font-size: 12px;"
        "  padding: 2px 6px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #4a90d9;"
        "}");
    m_input->setPlaceholderText("명령 입력 또는 좌표 (x,y / @dx,dy)...");

    inputRow->addWidget(m_promptLabel);
    inputRow->addWidget(m_input, 1);
    outerLayout->addLayout(inputRow);

    setFixedHeight(52);
    setStyleSheet("background-color: #252525;");

    // ── Connections ─────────────────────────────────────────
    connect(m_input, &QLineEdit::returnPressed,
            this, &CommandLineWidget::onReturnPressed);

    // Install event filter on the input to intercept arrow keys and ESC
    m_input->installEventFilter(this);
}

void CommandLineWidget::setPrompt(const QString& text)
{
    m_promptLabel->setText(text);
}

void CommandLineWidget::showMessage(const QString& text)
{
    m_messageLabel->setText(text);
}

void CommandLineWidget::focusInput()
{
    m_input->setFocus();
}

// ── Event filter ────────────────────────────────────────────

bool CommandLineWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != m_input || event->type() != QEvent::KeyPress)
        return QWidget::eventFilter(obj, event);

    auto* keyEvent = static_cast<QKeyEvent*>(event);

    switch (keyEvent->key()) {
    case Qt::Key_Up:
        historyUp();
        return true;
    case Qt::Key_Down:
        historyDown();
        return true;
    case Qt::Key_Escape:
        m_input->clear();
        m_historyIndex = -1;
        setPrompt("명령:");
        showMessage("");
        emit commandEntered("ESC");
        return true;
    default:
        break;
    }
    return QWidget::eventFilter(obj, event);
}

// ── Private helpers ──────────────────────────────────────────

void CommandLineWidget::onReturnPressed()
{
    const QString raw = m_input->text().trimmed();
    if (raw.isEmpty())
        return;

    // Add to history (avoid duplicates at the top)
    if (m_history.isEmpty() || m_history.last() != raw) {
        m_history.append(raw);
        if (m_history.size() > kMaxHistory)
            m_history.removeFirst();
    }
    m_historyIndex = -1;
    m_input->clear();

    // Try to parse as coordinate first
    double x = 0, y = 0;
    bool relative = false;
    if (parseCoordinate(raw, x, y, relative)) {
        if (relative)
            emit relativeCoordinateEntered(x, y);
        else
            emit coordinateEntered(x, y);
        return;
    }

    // Otherwise dispatch as a command string (upper-cased)
    emit commandEntered(raw.toUpper());
}

void CommandLineWidget::historyUp()
{
    if (m_history.isEmpty())
        return;

    if (m_historyIndex == -1)
        m_historyIndex = m_history.size(); // one past end

    if (m_historyIndex > 0) {
        --m_historyIndex;
        m_input->setText(m_history.at(m_historyIndex));
        m_input->selectAll();
    }
}

void CommandLineWidget::historyDown()
{
    if (m_history.isEmpty() || m_historyIndex == -1)
        return;

    ++m_historyIndex;
    if (m_historyIndex >= m_history.size()) {
        m_historyIndex = -1;
        m_input->clear();
    } else {
        m_input->setText(m_history.at(m_historyIndex));
        m_input->selectAll();
    }
}

/// Parse "x,y" or "@dx,dy".
/// Returns true if the text matches coordinate syntax.
bool CommandLineWidget::parseCoordinate(const QString& text,
                                        double& outX,
                                        double& outY,
                                        bool& outRelative)
{
    QString s = text.trimmed();
    outRelative = false;

    if (s.startsWith('@')) {
        outRelative = true;
        s = s.mid(1);
    }

    const int commaPos = s.indexOf(',');
    if (commaPos < 0)
        return false;

    bool okX = false, okY = false;
    const double x = s.left(commaPos).trimmed().toDouble(&okX);
    const double y = s.mid(commaPos + 1).trimmed().toDouble(&okY);

    if (!okX || !okY)
        return false;

    outX = x;
    outY = y;
    return true;
}

} // namespace yjcad
