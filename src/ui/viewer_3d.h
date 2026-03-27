#pragma once

#include "core/document/document.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QSlider>
#include <QVBoxLayout>
#include <memory>
#include <vector>

namespace yjcad {

/// 3D vertex: position (x,y,z) + color (r,g,b)
struct Vertex3D {
    float x, y, z;
    float r, g, b;
};

/// 3D Viewer — renders the 2D drawing in 3D with fold visualization
class Viewer3DWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    explicit Viewer3DWidget(Document& doc, QWidget* parent = nullptr);
    ~Viewer3DWidget() override;

    /// Rebuild 3D geometry from document
    void rebuildFromDocument();

public slots:
    void setFoldAngle(int angleDeg);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void buildGeometry();
    void buildFlatGeometry(std::vector<Vertex3D>& verts);
    void buildFoldedGeometry(std::vector<Vertex3D>& verts);
    void addLineSegment3D(std::vector<Vertex3D>& verts,
                          float x1, float y1, float z1,
                          float x2, float y2, float z2,
                          float r, float g, float b);
    void addGroundGrid(std::vector<Vertex3D>& verts);

    Document& m_doc;

    // OpenGL
    std::unique_ptr<QOpenGLShaderProgram> m_shader;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer};
    int m_vertexCount = 0;
    int m_mvpLoc = -1;
    bool m_initialized = false;

    // Camera
    float m_rotX = 30.0f;   // pitch (degrees)
    float m_rotY = -45.0f;  // yaw (degrees)
    float m_distance = 500.0f;
    QPointF m_lastMouse;

    // Fold
    float m_foldAngle = 0.0f; // 0 = flat, 90 = fully folded
};

/// Container widget with fold slider
class Viewer3DPanel : public QWidget {
    Q_OBJECT

public:
    explicit Viewer3DPanel(Document& doc, QWidget* parent = nullptr);
    Viewer3DWidget* viewer() { return m_viewer; }

private:
    Viewer3DWidget* m_viewer;
    QSlider* m_foldSlider;
};

} // namespace yjcad
