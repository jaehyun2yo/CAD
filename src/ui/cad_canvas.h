#pragma once

#include "core/renderer/gl_renderer.h"
#include "core/renderer/camera.h"
#include "core/document/document.h"
#include "ui/tool_manager.h"
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
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

signals:
    void cursorWorldPos(double x, double y);
    void entityClicked(EntityId id);
    void toolChanged(const QString& toolName);

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

    Document& m_doc;
    Camera m_camera;
    GlRenderer m_renderer;
    std::unique_ptr<ToolManager> m_toolMgr;

    // Mouse state
    bool m_panning = false;
    QPointF m_lastMousePos;
};

} // namespace yjcad
