#pragma once

#include "core/document/entity.h"
#include <QMatrix4x4>
#include <QPointF>
#include <QSizeF>

namespace yjcad {

/// Camera — manages the 2D viewport (zoom, pan, world↔screen transforms)
class Camera {
public:
    Camera();

    // ── Viewport ──
    void setViewportSize(int width, int height);
    int viewportWidth() const { return m_vpWidth; }
    int viewportHeight() const { return m_vpHeight; }

    // ── Zoom ──
    double zoom() const { return m_zoom; }
    void setZoom(double z);
    void zoomAt(double factor, const QPointF& screenPos);
    void zoomToFit(const BoundingBox& bounds, double margin = 0.1);

    // ── Pan ──
    Point2D center() const { return m_center; }
    void setCenter(const Point2D& c) { m_center = c; }
    void pan(const QPointF& screenDelta);

    // ── Transforms ──

    /// World coordinate → screen pixel
    QPointF worldToScreen(const Point2D& world) const;

    /// Screen pixel → world coordinate
    Point2D screenToWorld(const QPointF& screen) const;

    /// Get the visible world-space bounding box (for viewport culling)
    BoundingBox visibleBounds() const;

    /// Get the projection matrix for OpenGL
    QMatrix4x4 projectionMatrix() const;

private:
    void updateMatrix();

    Point2D m_center{0, 0};    // world-space center of view
    double m_zoom = 1.0;        // pixels per world unit
    int m_vpWidth = 800;
    int m_vpHeight = 600;
    QMatrix4x4 m_projMatrix;
};

} // namespace yjcad
