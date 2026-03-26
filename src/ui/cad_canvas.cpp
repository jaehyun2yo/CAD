#include "cad_canvas.h"
#include <cmath>
#include <limits>

namespace yjcad {

CadCanvas::CadCanvas(Document& doc, QWidget* parent)
    : QOpenGLWidget(parent), m_doc(doc) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_toolMgr = std::make_unique<ToolManager>(m_doc, this);
}

CadCanvas::~CadCanvas() {
    makeCurrent();
    m_renderer.cleanup();
    doneCurrent();
}

void CadCanvas::initializeGL() {
    if (!m_renderer.initialize()) {
        qWarning("Failed to initialize OpenGL renderer");
        return;
    }
    m_renderer.rebuildFromDocument(m_doc);
}

void CadCanvas::resizeGL(int w, int h) {
    m_camera.setViewportSize(w, h);
}

void CadCanvas::paintGL() {
    m_renderer.render(m_camera);
}

void CadCanvas::refreshEntities() {
    makeCurrent();
    m_renderer.rebuildFromDocument(m_doc);
    doneCurrent();
    update();
}

void CadCanvas::zoomToFit() {
    auto bounds = m_doc.bounds();
    m_camera.zoomToFit(bounds);
    update();
}

// ── Mouse events ──

void CadCanvas::mousePressEvent(QMouseEvent* event) {
    m_lastMousePos = event->position();

    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::LeftButton) {
        Point2D worldPos = m_camera.screenToWorld(event->position());

        // If a tool is active, forward to tool first
        if (m_toolMgr->currentMode() != ToolMode::Select) {
            if (m_toolMgr->mousePress(worldPos, Qt::LeftButton)) {
                update();
                return;
            }
        }

        // Default: Selection
        EntityId id = hitTest(event->position());
        if (id > 0) {
            if (event->modifiers() & Qt::ShiftModifier) {
                m_doc.selection().toggle(id);
            } else {
                m_doc.selection().clear();
                m_doc.selection().select(id);
            }
            refreshEntities();
            emit entityClicked(id);
        } else {
            if (!(event->modifiers() & Qt::ShiftModifier)) {
                m_doc.selection().clear();
                refreshEntities();
            }
        }
    } else if (event->button() == Qt::RightButton) {
        Point2D worldPos = m_camera.screenToWorld(event->position());
        m_toolMgr->mousePress(worldPos, Qt::RightButton);
        update();
    }
}

void CadCanvas::mouseMoveEvent(QMouseEvent* event) {
    QPointF pos = event->position();

    if (m_panning) {
        QPointF delta = pos - m_lastMousePos;
        m_camera.pan(delta);
        update();
    }

    m_lastMousePos = pos;

    // Forward to tool
    Point2D world = m_camera.screenToWorld(pos);
    if (m_toolMgr->mouseMove(world)) {
        update(); // Repaint for rubber-band preview
    }

    // Emit world coordinates for status bar
    emit cursorWorldPos(world.x, world.y);
}

void CadCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void CadCanvas::wheelEvent(QWheelEvent* event) {
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    m_camera.zoomAt(factor, event->position());
    update();
}

void CadCanvas::keyPressEvent(QKeyEvent* event) {
    if (m_toolMgr->keyPress(event->key())) {
        emit toolChanged("선택");
        update();
        return;
    }
    QOpenGLWidget::keyPressEvent(event);
}

// ── Hit test ──

EntityId CadCanvas::hitTest(const QPointF& screenPos, double tolerance) {
    Point2D worldPos = m_camera.screenToWorld(screenPos);
    double worldTol = tolerance / m_camera.zoom();

    EntityId closest = 0;
    double minDist = std::numeric_limits<double>::max();

    for (EntityId id : m_doc.entityIds()) {
        const Entity* entity = m_doc.entity(id);
        if (!entity || !entity->isVisible()) continue;

        double dist = std::numeric_limits<double>::max();

        switch (entity->type()) {
            case EntityType::Line: {
                auto* line = static_cast<const LineEntity*>(entity);
                // Point-to-line-segment distance
                Point2D a = line->start(), b = line->end();
                double dx = b.x - a.x, dy = b.y - a.y;
                double lenSq = dx * dx + dy * dy;
                if (lenSq < 1e-18) {
                    dist = worldPos.distanceTo(a);
                } else {
                    double t = std::clamp(((worldPos.x - a.x) * dx + (worldPos.y - a.y) * dy) / lenSq, 0.0, 1.0);
                    Point2D proj{a.x + t * dx, a.y + t * dy};
                    dist = worldPos.distanceTo(proj);
                }
                break;
            }
            case EntityType::Circle: {
                auto* circle = static_cast<const CircleEntity*>(entity);
                dist = std::abs(worldPos.distanceTo(circle->center()) - circle->radius());
                break;
            }
            case EntityType::Arc: {
                auto* arc = static_cast<const ArcEntity*>(entity);
                // Simplified: distance to circle, then check angle range
                dist = std::abs(worldPos.distanceTo(arc->center()) - arc->radius());
                break;
            }
            case EntityType::Polyline: {
                auto* poly = static_cast<const PolylineEntity*>(entity);
                const auto& verts = poly->vertices();
                for (size_t i = 0; i + 1 < verts.size(); ++i) {
                    Point2D a = verts[i], b = verts[i+1];
                    double dx = b.x - a.x, dy = b.y - a.y;
                    double lenSq = dx * dx + dy * dy;
                    double segDist;
                    if (lenSq < 1e-18) {
                        segDist = worldPos.distanceTo(a);
                    } else {
                        double t = std::clamp(((worldPos.x - a.x) * dx + (worldPos.y - a.y) * dy) / lenSq, 0.0, 1.0);
                        Point2D proj{a.x + t * dx, a.y + t * dy};
                        segDist = worldPos.distanceTo(proj);
                    }
                    dist = std::min(dist, segDist);
                }
                break;
            }
            default:
                break;
        }

        if (dist < worldTol && dist < minDist) {
            minDist = dist;
            closest = id;
        }
    }

    return closest;
}

} // namespace yjcad
