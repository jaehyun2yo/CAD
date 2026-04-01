#include "camera.h"
#include <algorithm>
#include <cmath>

namespace yjcad {

Camera::Camera() {
    updateMatrix();
}

void Camera::setViewportSize(int width, int height) {
    m_vpWidth = std::max(1, width);
    m_vpHeight = std::max(1, height);
    updateMatrix();
}

void Camera::setZoom(double z) {
    m_zoom = std::clamp(z, 0.001, 100000.0);
    updateMatrix();
}

void Camera::zoomAt(double factor, const QPointF& screenPos) {
    // Zoom while keeping the world point under the cursor fixed
    Point2D worldBefore = screenToWorld(screenPos);
    m_zoom = std::clamp(m_zoom * factor, 0.001, 100000.0);
    updateMatrix();
    Point2D worldAfter = screenToWorld(screenPos);
    // Adjust center to keep point under cursor
    m_center.x -= (worldAfter.x - worldBefore.x);
    m_center.y -= (worldAfter.y - worldBefore.y);
    updateMatrix();
}

void Camera::zoomToFit(const BoundingBox& bounds, double margin) {
    if (!bounds.isValid()) return;

    double bw = bounds.max.x - bounds.min.x;
    double bh = bounds.max.y - bounds.min.y;
    if (bw < 1e-9 && bh < 1e-9) return;

    // Add margin
    bw *= (1.0 + margin);
    bh *= (1.0 + margin);

    // Fit to viewport
    double zx = m_vpWidth / std::max(bw, 1e-9);
    double zy = m_vpHeight / std::max(bh, 1e-9);
    m_zoom = std::min(zx, zy);

    // Center on bounds
    m_center.x = (bounds.min.x + bounds.max.x) / 2.0;
    m_center.y = (bounds.min.y + bounds.max.y) / 2.0;
    updateMatrix();
}

void Camera::pan(const QPointF& screenDelta) {
    m_center.x -= screenDelta.x() / m_zoom;
    m_center.y += screenDelta.y() / m_zoom; // Y flipped in screen coords
    updateMatrix();
}

QPointF Camera::worldToScreen(const Point2D& world) const {
    double sx = (world.x - m_center.x) * m_zoom + m_vpWidth / 2.0;
    double sy = -(world.y - m_center.y) * m_zoom + m_vpHeight / 2.0; // Y flip
    return {sx, sy};
}

Point2D Camera::screenToWorld(const QPointF& screen) const {
    double wx = (screen.x() - m_vpWidth / 2.0) / m_zoom + m_center.x;
    double wy = -(screen.y() - m_vpHeight / 2.0) / m_zoom + m_center.y;
    return {wx, wy};
}

BoundingBox Camera::visibleBounds() const {
    Point2D topLeft = screenToWorld({0, 0});
    Point2D bottomRight = screenToWorld({
        static_cast<double>(m_vpWidth),
        static_cast<double>(m_vpHeight)
    });
    return {
        {std::min(topLeft.x, bottomRight.x), std::min(topLeft.y, bottomRight.y)},
        {std::max(topLeft.x, bottomRight.x), std::max(topLeft.y, bottomRight.y)}
    };
}

const QMatrix4x4& Camera::projectionMatrix() const {
    return m_projMatrix;
}

void Camera::updateMatrix() {
    m_projMatrix.setToIdentity();
    double halfW = m_vpWidth / (2.0 * m_zoom);
    double halfH = m_vpHeight / (2.0 * m_zoom);
    m_projMatrix.ortho(
        static_cast<float>(m_center.x - halfW),
        static_cast<float>(m_center.x + halfW),
        static_cast<float>(m_center.y - halfH),
        static_cast<float>(m_center.y + halfH),
        -1.0f, 1.0f
    );
}

} // namespace yjcad
