#include "cad_canvas.h"
#include <cmath>
#include <limits>

namespace yjcad {

CadCanvas::CadCanvas(Document& doc, QWidget* parent)
    : QOpenGLWidget(parent), m_doc(doc) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_toolMgr = std::make_unique<ToolManager>(m_doc, this);
    m_snap = std::make_unique<SnapEngine>(m_doc);
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

    // Text overlay: render text entities using QPainter on top of OpenGL
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    renderTextOverlay(painter);
    painter.end();
}

void CadCanvas::renderTextOverlay(QPainter& painter) {
    // Use cached text entity list instead of iterating all entities
    if (m_textIdsDirty) rebuildTextEntityCache();
    if (m_textEntityIds.empty()) return;

    // Pre-compute viewport bounds for culling
    auto vpBounds = m_camera.visibleBounds();
    int vpW = width(), vpH = height();

    // Reuse a single QFont object (avoid per-entity allocation)
    QFont font("Malgun Gothic", 12);
    font.setStyleHint(QFont::SansSerif);
    int lastFontSize = 12;

    for (EntityId id : m_textEntityIds) {
        const Entity* entity = m_doc.entity(id);
        if (!entity || !entity->isVisible()) continue;

        auto* text = static_cast<const TextEntity*>(entity);

        // Viewport culling: skip text outside visible bounds
        const auto& pos = text->position();
        if (pos.x < vpBounds.min.x || pos.x > vpBounds.max.x ||
            pos.y < vpBounds.min.y || pos.y > vpBounds.max.y) continue;

        QPointF screenPos = m_camera.worldToScreen(pos);

        // Skip if off-screen (with margin for text width)
        if (screenPos.x() < -200 || screenPos.x() > vpW + 200 ||
            screenPos.y() < -200 || screenPos.y() > vpH + 200) continue;

        // Compute font size from world height * zoom
        double fontSize = text->height() * m_camera.zoom();
        if (fontSize < 4.0) continue; // too small to read
        fontSize = std::min(fontSize, 200.0); // cap max size

        int intFontSize = static_cast<int>(fontSize);
        if (intFontSize != lastFontSize) {
            font.setPointSize(intFontSize);
            painter.setFont(font);
            lastFontSize = intFontSize;
        }

        // Color based on selection/classification
        QColor color;
        if (entity->isSelected()) {
            color = QColor(0, 255, 255); // cyan
        } else if (m_darkTheme) {
            color = QColor(200, 200, 200); // light gray on dark
        } else {
            color = QColor(50, 50, 50); // dark gray on light
        }
        painter.setPen(color);

        // Apply rotation if any
        if (std::abs(text->rotation()) > 0.01) {
            painter.save();
            painter.translate(screenPos);
            painter.rotate(-text->rotation()); // screen Y is flipped
            painter.drawText(QPointF(0, 0), QString::fromStdString(text->text()));
            painter.restore();
        } else {
            painter.drawText(screenPos, QString::fromStdString(text->text()));
        }
    }
}

void CadCanvas::setOrthoMode(bool enabled) {
    m_orthoMode = enabled;
    emit orthoModeChanged(enabled);
    update();
}

void CadCanvas::toggleOrthoMode() {
    setOrthoMode(!m_orthoMode);
}

void CadCanvas::setSnapEnabled(bool enabled) {
    m_snapEnabled = enabled;
    emit snapEnabledChanged(enabled);
}

void CadCanvas::toggleSnap() {
    setSnapEnabled(!m_snapEnabled);
}

void CadCanvas::setLastCommittedPoint(const Point2D& p) {
    m_lastCommittedPt = p;
    m_hasLastCommittedPt = true;
    m_snap->setReferencePoint(p);
}

void CadCanvas::setDarkTheme(bool dark) {
    m_darkTheme = dark;
    makeCurrent();
    if (dark) {
        m_renderer.setBackgroundColor(0.0f, 0.0f, 0.0f);
    } else {
        m_renderer.setBackgroundColor(1.0f, 1.0f, 1.0f);
    }
    doneCurrent();
    update();
}

void CadCanvas::refreshEntities() {
    makeCurrent();
    m_renderer.rebuildFromDocument(m_doc);
    doneCurrent();
    m_textIdsDirty = true;
    update();
}

void CadCanvas::rebuildTextEntityCache() {
    m_textEntityIds.clear();
    for (EntityId id : m_doc.entityIds()) {
        const Entity* e = m_doc.entity(id);
        if (e && e->type() == EntityType::Text) {
            m_textEntityIds.push_back(id);
        }
    }
    m_textIdsDirty = false;
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

        // Ortho constraint
        if (m_orthoMode && m_hasLastCommittedPt) {
            double dx = std::abs(worldPos.x - m_lastCommittedPt.x);
            double dy = std::abs(worldPos.y - m_lastCommittedPt.y);
            if (dx >= dy) {
                worldPos.y = m_lastCommittedPt.y;
            } else {
                worldPos.x = m_lastCommittedPt.x;
            }
        }

        // Apply snap
        if (m_snapEnabled) {
            double snapTol = 10.0 / m_camera.zoom();
            auto snapResult = m_snap->snap(worldPos, snapTol);
            if (snapResult.valid) worldPos = snapResult.point;
        }

        // If a tool is active, forward to tool first
        if (m_toolMgr->currentMode() != ToolMode::Select) {
            if (m_toolMgr->mousePress(worldPos, Qt::LeftButton)) {
                // Record committed point for next snap/ortho
                setLastCommittedPoint(worldPos);
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

    // Forward to tool (with snap and ortho)
    Point2D world = m_camera.screenToWorld(pos);

    // Ortho constraint: align to horizontal or vertical from last committed point
    if (m_orthoMode && m_hasLastCommittedPt) {
        double dx = std::abs(world.x - m_lastCommittedPt.x);
        double dy = std::abs(world.y - m_lastCommittedPt.y);
        if (dx >= dy) {
            world.y = m_lastCommittedPt.y; // horizontal axis
        } else {
            world.x = m_lastCommittedPt.x; // vertical axis
        }
    }

    double snapTol = 10.0 / m_camera.zoom(); // 10 screen pixels
    SnapResult snapResult;
    if (m_snapEnabled) {
        snapResult = m_snap->snap(world, snapTol);
    }
    Point2D snappedWorld = snapResult.valid ? snapResult.point : world;

    if (m_toolMgr->mouseMove(snappedWorld)) {
        // Update rubber-band preview in renderer
        auto* tool = m_toolMgr->currentTool();
        if (tool) {
            const Point2D* snapPt = snapResult.valid ? &snapResult.point : nullptr;
            makeCurrent();
            m_renderer.updatePreview(tool->previewLines(), snapPt);
            doneCurrent();
        }
        update();
    }

    // Emit world coordinates for status bar
    emit cursorWorldPos(snappedWorld.x, snappedWorld.y);
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
    switch (event->key()) {
        case Qt::Key_F3:
            toggleSnap();
            update();
            return;
        case Qt::Key_F8:
            toggleOrthoMode();
            update();
            return;
        default:
            break;
    }

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
