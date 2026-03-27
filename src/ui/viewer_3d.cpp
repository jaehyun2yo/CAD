#include "viewer_3d.h"
#include <QLabel>
#include <cmath>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;

// ── Viewer3DWidget ──

Viewer3DWidget::Viewer3DWidget(Document& doc, QWidget* parent)
    : QOpenGLWidget(parent), m_doc(doc) {
    setMinimumSize(300, 200);

    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    fmt.setDepthBufferSize(24);
    setFormat(fmt);
}

Viewer3DWidget::~Viewer3DWidget() {
    if (m_initialized) {
        makeCurrent();
        m_vbo.destroy();
        m_vao.destroy();
        m_shader.reset();
        doneCurrent();
    }
}

void Viewer3DWidget::initializeGL() {
    initializeOpenGLFunctions();

    m_shader = std::make_unique<QOpenGLShaderProgram>();

    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        uniform mat4 uMVP;
        out vec3 vColor;
        void main() {
            gl_Position = uMVP * vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";

    const char* fragSrc = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc);
    m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc);
    m_shader->link();
    m_mvpLoc = m_shader->uniformLocation("uMVP");

    m_vao.create();
    m_vbo.create();

    m_initialized = true;

    // Auto-fit distance based on document bounds
    auto bounds = m_doc.bounds();
    double bw = bounds.max.x - bounds.min.x;
    double bh = bounds.max.y - bounds.min.y;
    m_distance = static_cast<float>(std::max(bw, bh) * 1.5);
    if (m_distance < 100.0f) m_distance = 500.0f;

    rebuildFromDocument();
}

void Viewer3DWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void Viewer3DWidget::paintGL() {
    if (!m_initialized) return;

    glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.5f);

    // Build MVP matrix
    QMatrix4x4 projection;
    float aspect = static_cast<float>(width()) / std::max(1, height());
    projection.perspective(45.0f, aspect, 0.1f, m_distance * 10.0f);

    QMatrix4x4 view;
    float radX = m_rotX * static_cast<float>(PI / 180.0);
    float radY = m_rotY * static_cast<float>(PI / 180.0);
    float eyeX = m_distance * std::cos(radX) * std::sin(radY);
    float eyeY = m_distance * std::sin(radX);
    float eyeZ = m_distance * std::cos(radX) * std::cos(radY);

    // Center on document bounds
    auto bounds = m_doc.bounds();
    float cx = static_cast<float>((bounds.min.x + bounds.max.x) / 2.0);
    float cy = static_cast<float>((bounds.min.y + bounds.max.y) / 2.0);
    view.lookAt(QVector3D(cx + eyeX, cy + eyeY, eyeZ),
                QVector3D(cx, cy, 0.0f),
                QVector3D(0, 1, 0));

    QMatrix4x4 mvp = projection * view;

    m_shader->bind();
    m_shader->setUniformValue(m_mvpLoc, mvp);

    if (m_vertexCount > 0) {
        m_vao.bind();
        glDrawArrays(GL_LINES, 0, m_vertexCount);
        m_vao.release();
    }

    m_shader->release();
}

void Viewer3DWidget::rebuildFromDocument() {
    if (!m_initialized) return;

    std::vector<Vertex3D> verts;
    buildGeometry();
    update();
}

void Viewer3DWidget::setFoldAngle(int angleDeg) {
    m_foldAngle = static_cast<float>(angleDeg);
    if (m_initialized) {
        makeCurrent();
        buildGeometry();
        doneCurrent();
        update();
    }
}

void Viewer3DWidget::buildGeometry() {
    std::vector<Vertex3D> verts;

    // Ground grid
    addGroundGrid(verts);

    // Build entities (flat or folded based on fold angle)
    if (std::abs(m_foldAngle) < 0.1f) {
        buildFlatGeometry(verts);
    } else {
        buildFoldedGeometry(verts);
    }

    m_vertexCount = static_cast<int>(verts.size());

    m_vao.bind();
    m_vbo.bind();
    if (!verts.empty()) {
        m_vbo.allocate(verts.data(), static_cast<int>(verts.size() * sizeof(Vertex3D)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                              reinterpret_cast<void*>(3 * sizeof(float)));
    }
    m_vbo.release();
    m_vao.release();
}

void Viewer3DWidget::buildFlatGeometry(std::vector<Vertex3D>& verts) {
    // Render all entities flat on z=0 plane
    for (EntityId id : m_doc.entityIds()) {
        const Entity* entity = m_doc.entity(id);
        if (!entity || !entity->isVisible()) continue;

        float r = 0.8f, g = 0.8f, b = 0.8f;
        switch (entity->classification()) {
        case LineClassification::Oshi:  r = 1.0f; g = 0.0f; b = 0.0f; break;
        case LineClassification::Knife: r = 1.0f; g = 1.0f; b = 1.0f; break;
        case LineClassification::Guide: r = 0.0f; g = 1.0f; b = 0.0f; break;
        default: break;
        }

        switch (entity->type()) {
        case EntityType::Line: {
            auto* line = static_cast<const LineEntity*>(entity);
            addLineSegment3D(verts,
                static_cast<float>(line->start().x), static_cast<float>(line->start().y), 0,
                static_cast<float>(line->end().x), static_cast<float>(line->end().y), 0,
                r, g, b);
            break;
        }
        case EntityType::Circle: {
            auto* circle = static_cast<const CircleEntity*>(entity);
            float cx = static_cast<float>(circle->center().x);
            float cy = static_cast<float>(circle->center().y);
            float rad = static_cast<float>(circle->radius());
            for (int i = 0; i < 48; ++i) {
                float a1 = static_cast<float>(2.0 * PI * i / 48);
                float a2 = static_cast<float>(2.0 * PI * (i + 1) / 48);
                addLineSegment3D(verts,
                    cx + rad * std::cos(a1), cy + rad * std::sin(a1), 0,
                    cx + rad * std::cos(a2), cy + rad * std::sin(a2), 0,
                    r, g, b);
            }
            break;
        }
        case EntityType::Polyline: {
            auto* poly = static_cast<const PolylineEntity*>(entity);
            const auto& pts = poly->vertices();
            for (size_t i = 0; i + 1 < pts.size(); ++i) {
                addLineSegment3D(verts,
                    static_cast<float>(pts[i].x), static_cast<float>(pts[i].y), 0,
                    static_cast<float>(pts[i+1].x), static_cast<float>(pts[i+1].y), 0,
                    r, g, b);
            }
            if (poly->isClosed() && pts.size() > 1) {
                addLineSegment3D(verts,
                    static_cast<float>(pts.back().x), static_cast<float>(pts.back().y), 0,
                    static_cast<float>(pts.front().x), static_cast<float>(pts.front().y), 0,
                    r, g, b);
            }
            break;
        }
        default: break;
        }
    }
}

void Viewer3DWidget::buildFoldedGeometry(std::vector<Vertex3D>& verts) {
    // Find 오시선 (fold lines) — entities classified as Oshi
    // For simplicity: entities above an oshi line fold up, below fold down
    // This is a simplified fold visualization

    float foldRad = m_foldAngle * static_cast<float>(PI / 180.0);

    // Find oshi lines and their Y positions
    std::vector<double> foldYs;
    for (EntityId id : m_doc.entityIds()) {
        const Entity* e = m_doc.entity(id);
        if (!e || e->classification() != LineClassification::Oshi) continue;
        if (e->type() == EntityType::Line) {
            auto* line = static_cast<const LineEntity*>(e);
            // Use midpoint Y as fold axis
            double midY = (line->start().y + line->end().y) / 2.0;
            foldYs.push_back(midY);
        }
    }

    // Sort fold lines
    std::sort(foldYs.begin(), foldYs.end());

    // For each entity, determine which fold zone it's in and apply rotation
    for (EntityId id : m_doc.entityIds()) {
        const Entity* entity = m_doc.entity(id);
        if (!entity || !entity->isVisible()) continue;

        float r = 0.8f, g = 0.8f, b = 0.8f;
        switch (entity->classification()) {
        case LineClassification::Oshi:  r = 1.0f; g = 0.0f; b = 0.0f; break;
        case LineClassification::Knife: r = 1.0f; g = 1.0f; b = 1.0f; break;
        case LineClassification::Guide: r = 0.0f; g = 1.0f; b = 0.0f; break;
        default: break;
        }

        // Lambda to transform a 2D point to 3D with folding
        auto transform = [&](double wx, double wy) -> std::tuple<float, float, float> {
            float x = static_cast<float>(wx);
            float y = static_cast<float>(wy);
            float z = 0.0f;

            // Find which fold zone this point belongs to
            int zone = 0;
            for (double foldY : foldYs) {
                if (wy > foldY) zone++;
            }

            // Accumulate rotations from each fold line
            if (zone > 0 && !foldYs.empty()) {
                // Fold around the nearest lower fold line
                double pivotY = foldYs[std::min(zone - 1, static_cast<int>(foldYs.size()) - 1)];
                float dy = y - static_cast<float>(pivotY);
                float angle = foldRad * zone;
                // Rotate (y, z) around the fold axis
                float newDy = dy * std::cos(angle);
                float newZ = dy * std::sin(angle);
                y = static_cast<float>(pivotY) + newDy;
                z = newZ;
            }

            return {x, y, z};
        };

        // Apply to all entity types
        if (entity->type() == EntityType::Line) {
            auto* line = static_cast<const LineEntity*>(entity);
            auto [x1, y1, z1] = transform(line->start().x, line->start().y);
            auto [x2, y2, z2] = transform(line->end().x, line->end().y);
            addLineSegment3D(verts, x1, y1, z1, x2, y2, z2, r, g, b);
        }
        else if (entity->type() == EntityType::Polyline) {
            auto* poly = static_cast<const PolylineEntity*>(entity);
            const auto& pts = poly->vertices();
            for (size_t i = 0; i + 1 < pts.size(); ++i) {
                auto [x1, y1, z1] = transform(pts[i].x, pts[i].y);
                auto [x2, y2, z2] = transform(pts[i+1].x, pts[i+1].y);
                addLineSegment3D(verts, x1, y1, z1, x2, y2, z2, r, g, b);
            }
            if (poly->isClosed() && pts.size() > 1) {
                auto [x1, y1, z1] = transform(pts.back().x, pts.back().y);
                auto [x2, y2, z2] = transform(pts.front().x, pts.front().y);
                addLineSegment3D(verts, x1, y1, z1, x2, y2, z2, r, g, b);
            }
        }
        else if (entity->type() == EntityType::Circle) {
            auto* circle = static_cast<const CircleEntity*>(entity);
            double cx = circle->center().x;
            double cy = circle->center().y;
            double rad = circle->radius();
            for (int i = 0; i < 48; ++i) {
                double a1 = 2.0 * PI * i / 48;
                double a2 = 2.0 * PI * (i + 1) / 48;
                auto [x1, y1, z1] = transform(cx + rad * std::cos(a1), cy + rad * std::sin(a1));
                auto [x2, y2, z2] = transform(cx + rad * std::cos(a2), cy + rad * std::sin(a2));
                addLineSegment3D(verts, x1, y1, z1, x2, y2, z2, r, g, b);
            }
        }
    }
}

void Viewer3DWidget::addLineSegment3D(std::vector<Vertex3D>& verts,
                                       float x1, float y1, float z1,
                                       float x2, float y2, float z2,
                                       float r, float g, float b) {
    verts.push_back({x1, y1, z1, r, g, b});
    verts.push_back({x2, y2, z2, r, g, b});
}

void Viewer3DWidget::addGroundGrid(std::vector<Vertex3D>& verts) {
    auto bounds = m_doc.bounds();
    float minX = static_cast<float>(bounds.min.x - 50);
    float maxX = static_cast<float>(bounds.max.x + 50);
    float minY = static_cast<float>(bounds.min.y - 50);
    float maxY = static_cast<float>(bounds.max.y + 50);
    float step = 50.0f;

    float gr = 0.15f, gg = 0.15f, gb = 0.15f;
    for (float x = std::floor(minX / step) * step; x <= maxX; x += step) {
        addLineSegment3D(verts, x, minY, -0.1f, x, maxY, -0.1f, gr, gg, gb);
    }
    for (float y = std::floor(minY / step) * step; y <= maxY; y += step) {
        addLineSegment3D(verts, minX, y, -0.1f, maxX, y, -0.1f, gr, gg, gb);
    }

    // Origin axes
    addLineSegment3D(verts, minX, 0, 0, maxX, 0, 0, 0.3f, 0, 0);   // X red
    addLineSegment3D(verts, 0, minY, 0, 0, maxY, 0, 0, 0.3f, 0);   // Y green
    addLineSegment3D(verts, 0, 0, -50, 0, 0, 200, 0, 0, 0.3f);     // Z blue
}

void Viewer3DWidget::mousePressEvent(QMouseEvent* event) {
    m_lastMouse = event->position();
}

void Viewer3DWidget::mouseMoveEvent(QMouseEvent* event) {
    QPointF delta = event->position() - m_lastMouse;
    m_lastMouse = event->position();

    if (event->buttons() & Qt::LeftButton) {
        m_rotY += static_cast<float>(delta.x()) * 0.5f;
        m_rotX += static_cast<float>(delta.y()) * 0.5f;
        m_rotX = std::clamp(m_rotX, -89.0f, 89.0f);
        update();
    } else if (event->buttons() & Qt::MiddleButton) {
        // Pan not implemented in 3D — just zoom
    }
}

void Viewer3DWidget::wheelEvent(QWheelEvent* event) {
    float factor = event->angleDelta().y() > 0 ? 0.9f : 1.1f;
    m_distance *= factor;
    m_distance = std::clamp(m_distance, 10.0f, 100000.0f);
    update();
}

// ── Viewer3DPanel ──

Viewer3DPanel::Viewer3DPanel(Document& doc, QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_viewer = new Viewer3DWidget(doc, this);
    layout->addWidget(m_viewer, 1);

    // Fold angle slider
    auto* sliderRow = new QHBoxLayout();
    auto* label = new QLabel("접기 각도:");
    label->setStyleSheet("color: #cccccc; padding: 0 4px;");
    m_foldSlider = new QSlider(Qt::Horizontal);
    m_foldSlider->setRange(0, 180);
    m_foldSlider->setValue(0);
    m_foldSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #3a3a3a; height: 6px; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #4a90d9; width: 14px; margin: -4px 0; "
        "border-radius: 7px; }");
    auto* angleLabel = new QLabel("0°");
    angleLabel->setStyleSheet("color: #aaaaaa; min-width: 30px;");

    connect(m_foldSlider, &QSlider::valueChanged, [this, angleLabel](int val) {
        angleLabel->setText(QString("%1°").arg(val));
        m_viewer->setFoldAngle(val);
    });

    sliderRow->addWidget(label);
    sliderRow->addWidget(m_foldSlider, 1);
    sliderRow->addWidget(angleLabel);
    layout->addLayout(sliderRow);
}

} // namespace yjcad
