#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QStringList>

namespace yjcad {

/// DreamCAD-style command line input widget.
/// Placed at the bottom of the main window.
/// Supports history navigation (↑/↓), ESC to cancel,
/// coordinate input ("100,200" or "@50,30"), and autocomplete.
class CommandLineWidget : public QWidget {
    Q_OBJECT

public:
    explicit CommandLineWidget(QWidget* parent = nullptr);

    /// Show a prompt message (e.g., "점을 지정하세요:")
    void setPrompt(const QString& text);

    /// Append a message to the output label
    void showMessage(const QString& text);

    /// Give focus to the input field
    void focusInput();

signals:
    /// Emitted when the user presses Enter with a non-coordinate string
    void commandEntered(const QString& cmd);

    /// Emitted when absolute coordinate input is detected ("100,200")
    void coordinateEntered(double x, double y);

    /// Emitted when relative coordinate input is detected ("@50,30")
    void relativeCoordinateEntered(double dx, double dy);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void onReturnPressed();
    void historyUp();
    void historyDown();
    bool parseCoordinate(const QString& text, double& outX, double& outY, bool& outRelative);

    QLabel*    m_promptLabel  = nullptr;
    QLineEdit* m_input        = nullptr;
    QLabel*    m_messageLabel = nullptr;

    QStringList m_history;          // command history (oldest first)
    int         m_historyIndex = -1; // -1 = not navigating
    static constexpr int kMaxHistory = 50;
};

} // namespace yjcad
