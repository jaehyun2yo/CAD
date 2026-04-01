#pragma once

#include "core/renderer/gl_renderer.h"
#include "core/renderer/camera.h"
#include "core/renderer/snap.h"
#include "core/document/document.h"
#include "ui/tool_manager.h"
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QElapsedTimer>
#include <memory>

namespace yjcad {

/// CAD Canvas — QOpenGLWidget that renders and interacts with the document
class CadCanvas : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit CadCanvas(Document& doc, QWidget* parent = nullptr);
    ~CadCanvas() override;

    Camera& camera() { return m_camera; }
    ToolManager& toolManager() { return *m_toolMgr; }
    void zoomToFit();
    void refreshEntities();
    void refreshSelectionColors();

    /// Theme control
    void setDarkTheme(bool dark);
    bool isDarkTheme() const { return m_darkTheme; }

    /// Ortho mode — constrain cursor to X/Y axis relative to last committed point
    void setOrthoMode(bool enabled);
    bool isOrthoMode() const { return m_orthoMode; }
    void toggleOrthoMode();

    /// Snap enabled/disabled toggle (all snap types at once)
    void setSnapEnabled(bool enabled);
    bool isSnapEnabled() const { return m_snapEnabled; }
    void toggleSnap();

    /// Notify canvas of the last committed drawing point (for Ortho & Perp/Tan snaps)
    void setLastCommittedPoint(const Point2D& p);

signals:
    void cursorWorldPos(double x, double y);
    void entityClicked(EntityId id);
    void toolChanged(const QString& toolName);
    void orthoModeChanged(bool enabled);
    void snapEnabledChanged(bool enabled);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    EntityId hitTest(const QPointF& screenPos, double tolerance = 5.0);
    void renderTextOverlay(QPainter& painter);

    Document& m_doc;
    Camera m_camera;
    GlRenderer m_renderer;
    std::unique_ptr<ToolManager> m_toolMgr;
    std::unique_ptr<SnapEngine> m_snap;

    // Mouse state
    bool m_panning = false;
    QPointF m_lastMousePos;

    // Theme
    bool m_darkTheme = true;

    // Ortho mode
    bool m_orthoMode = false;

    // Snap master switch
    bool m_snapEnabled = true;

    // Last committed point used for Ortho constraint and Perp/Tan snaps
    Point2D m_lastCommittedPt;
    bool    m_hasLastCommittedPt = false;

    // Cached text entity IDs — rebuilt when entities change, not every frame
    std::vector<EntityId> m_textEntityIds;
    bool m_textIdsDirty = true;
    void rebuildTextEntityCache();

    // Zoom debounce — suppress snap during active zooming
    QElapsedTimer m_zoomTimer;
    bool m_isZooming = false;
    static constexpr int ZOOM_DEBOUNCE_MS = 150;
    bool isActivelyZooming() const;
};

} // namespace yjcad
