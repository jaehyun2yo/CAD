#include "entity.h"
#include <cmath>
#include <algorithm>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;
static constexpr double DEG2RAD = PI / 180.0;

// Helper: rotate a point around center
static Point2D rotatePoint(const Point2D& p, const Point2D& center, double angleDeg) {
    double rad = angleDeg * DEG2RAD;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);
    double dx = p.x - center.x;
    double dy = p.y - center.y;
    return {
        center.x + dx * cosA - dy * sinA,
        center.y + dx * sinA + dy * cosA
    };
}

// Helper: mirror a point across line p1-p2
static Point2D mirrorPoint(const Point2D& p, const Point2D& p1, const Point2D& p2) {
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double lenSq = dx * dx + dy * dy;
    if (lenSq < 1e-18) return p; // degenerate line
    double t = ((p.x - p1.x) * dx + (p.y - p1.y) * dy) / lenSq;
    Point2D proj = {p1.x + t * dx, p1.y + t * dy};
    return {2.0 * proj.x - p.x, 2.0 * proj.y - p.y};
}

// Helper: scale a point from center
static Point2D scalePoint(const Point2D& p, const Point2D& center, double factor) {
    return {
        center.x + (p.x - center.x) * factor,
        center.y + (p.y - center.y) * factor
    };
}

// ── Entity base ──

Entity::Entity(EntityType type, EntityId id)
    : m_id(id), m_type(type) {}

// ── LineEntity ──

LineEntity::LineEntity(EntityId id, const Point2D& start, const Point2D& end)
    : Entity(EntityType::Line, id), m_start(start), m_end(end) {}

BoundingBox LineEntity::boundingBox() const {
    return {
        {std::min(m_start.x, m_end.x), std::min(m_start.y, m_end.y)},
        {std::max(m_start.x, m_end.x), std::max(m_start.y, m_end.y)}
    };
}

std::unique_ptr<Entity> LineEntity::clone() const {
    auto e = std::make_unique<LineEntity>(id(), m_start, m_end);
    e->setLayerName(layerName());
    e->setColor(color());
    e->setLineWidth(lineWidth());
    e->setClassification(classification());
    return e;
}

void LineEntity::translate(const Point2D& delta) {
    m_start = m_start + delta;
    m_end = m_end + delta;
}

void LineEntity::rotate(const Point2D& center, double angleDeg) {
    m_start = rotatePoint(m_start, center, angleDeg);
    m_end = rotatePoint(m_end, center, angleDeg);
}

void LineEntity::mirror(const Point2D& p1, const Point2D& p2) {
    m_start = mirrorPoint(m_start, p1, p2);
    m_end = mirrorPoint(m_end, p1, p2);
}

void LineEntity::scale(const Point2D& center, double factor) {
    m_start = scalePoint(m_start, center, factor);
    m_end = scalePoint(m_end, center, factor);
}

// ── CircleEntity ──

CircleEntity::CircleEntity(EntityId id, const Point2D& center, double radius)
    : Entity(EntityType::Circle, id), m_center(center), m_radius(radius) {}

BoundingBox CircleEntity::boundingBox() const {
    return {
        {m_center.x - m_radius, m_center.y - m_radius},
        {m_center.x + m_radius, m_center.y + m_radius}
    };
}

std::unique_ptr<Entity> CircleEntity::clone() const {
    auto e = std::make_unique<CircleEntity>(id(), m_center, m_radius);
    e->setLayerName(layerName());
    e->setColor(color());
    e->setLineWidth(lineWidth());
    e->setClassification(classification());
    return e;
}

void CircleEntity::translate(const Point2D& delta) {
    m_center = m_center + delta;
}

void CircleEntity::rotate(const Point2D& center, double angleDeg) {
    m_center = rotatePoint(m_center, center, angleDeg);
}

void CircleEntity::mirror(const Point2D& p1, const Point2D& p2) {
    m_center = mirrorPoint(m_center, p1, p2);
}

void CircleEntity::scale(const Point2D& center, double factor) {
    m_center = scalePoint(m_center, center, factor);
    m_radius *= std::abs(factor);
}

// ── ArcEntity ──

ArcEntity::ArcEntity(EntityId id, const Point2D& center, double radius,
                     double startAngleDeg, double endAngleDeg)
    : Entity(EntityType::Arc, id), m_center(center), m_radius(radius),
      m_startAngle(startAngleDeg), m_endAngle(endAngleDeg) {}

BoundingBox ArcEntity::boundingBox() const {
    // Conservative: use circle bounding box
    // TODO: compute tight arc bounding box
    return {
        {m_center.x - m_radius, m_center.y - m_radius},
        {m_center.x + m_radius, m_center.y + m_radius}
    };
}

std::unique_ptr<Entity> ArcEntity::clone() const {
    auto e = std::make_unique<ArcEntity>(id(), m_center, m_radius, m_startAngle, m_endAngle);
    e->setLayerName(layerName());
    e->setColor(color());
    e->setLineWidth(lineWidth());
    e->setClassification(classification());
    return e;
}

void ArcEntity::translate(const Point2D& delta) {
    m_center = m_center + delta;
}

void ArcEntity::rotate(const Point2D& center, double angleDeg) {
    m_center = rotatePoint(m_center, center, angleDeg);
    m_startAngle += angleDeg;
    m_endAngle += angleDeg;
}

void ArcEntity::mirror(const Point2D& p1, const Point2D& p2) {
    m_center = mirrorPoint(m_center, p1, p2);
    // Mirror reverses angle direction
    double lineAngle = std::atan2(p2.y - p1.y, p2.x - p1.x) / DEG2RAD;
    m_startAngle = 2.0 * lineAngle - m_startAngle;
    m_endAngle = 2.0 * lineAngle - m_endAngle;
    std::swap(m_startAngle, m_endAngle);
}

void ArcEntity::scale(const Point2D& center, double factor) {
    m_center = scalePoint(m_center, center, factor);
    m_radius *= std::abs(factor);
}

// ── PolylineEntity ──

PolylineEntity::PolylineEntity(EntityId id, const std::vector<Point2D>& vertices, bool closed)
    : Entity(EntityType::Polyline, id), m_vertices(vertices), m_closed(closed) {}

BoundingBox PolylineEntity::boundingBox() const {
    if (m_vertices.empty()) return {{0, 0}, {0, 0}};
    BoundingBox bb = {{m_vertices[0].x, m_vertices[0].y},
                      {m_vertices[0].x, m_vertices[0].y}};
    for (size_t i = 1; i < m_vertices.size(); ++i) {
        bb.expand(m_vertices[i]);
    }
    return bb;
}

std::unique_ptr<Entity> PolylineEntity::clone() const {
    auto e = std::make_unique<PolylineEntity>(id(), m_vertices, m_closed);
    e->setLayerName(layerName());
    e->setColor(color());
    e->setLineWidth(lineWidth());
    e->setClassification(classification());
    return e;
}

void PolylineEntity::translate(const Point2D& delta) {
    for (auto& v : m_vertices) v = v + delta;
}

void PolylineEntity::rotate(const Point2D& center, double angleDeg) {
    for (auto& v : m_vertices) v = rotatePoint(v, center, angleDeg);
}

void PolylineEntity::mirror(const Point2D& p1, const Point2D& p2) {
    for (auto& v : m_vertices) v = mirrorPoint(v, p1, p2);
}

void PolylineEntity::scale(const Point2D& center, double factor) {
    for (auto& v : m_vertices) v = scalePoint(v, center, factor);
}

// ── TextEntity ──

TextEntity::TextEntity(EntityId id, const Point2D& position,
                       const std::string& text, double height)
    : Entity(EntityType::Text, id), m_position(position),
      m_text(text), m_height(height) {}

BoundingBox TextEntity::boundingBox() const {
    // Approximate: width = height * text length * 0.6
    double approxWidth = m_height * static_cast<double>(m_text.size()) * 0.6;
    return {
        {m_position.x, m_position.y},
        {m_position.x + approxWidth, m_position.y + m_height}
    };
}

std::unique_ptr<Entity> TextEntity::clone() const {
    auto e = std::make_unique<TextEntity>(id(), m_position, m_text, m_height);
    e->setRotation(m_rotation);
    e->setLayerName(layerName());
    e->setColor(color());
    e->setClassification(classification());
    return e;
}

void TextEntity::translate(const Point2D& delta) {
    m_position = m_position + delta;
}

void TextEntity::rotate(const Point2D& center, double angleDeg) {
    m_position = rotatePoint(m_position, center, angleDeg);
    m_rotation += angleDeg;
}

void TextEntity::mirror(const Point2D& p1, const Point2D& p2) {
    m_position = mirrorPoint(m_position, p1, p2);
}

void TextEntity::scale(const Point2D& center, double factor) {
    m_position = scalePoint(m_position, center, factor);
    m_height *= std::abs(factor);
}

} // namespace yjcad
