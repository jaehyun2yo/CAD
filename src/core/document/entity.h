#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <cmath>

namespace yjcad {

/// 2D point
struct Point2D {
    double x = 0.0;
    double y = 0.0;

    Point2D() = default;
    Point2D(double x, double y) : x(x), y(y) {}

    bool operator==(const Point2D& other) const {
        constexpr double eps = 1e-9;
        return std::abs(x - other.x) < eps && std::abs(y - other.y) < eps;
    }

    bool operator!=(const Point2D& other) const { return !(*this == other); }

    Point2D operator+(const Point2D& other) const { return {x + other.x, y + other.y}; }
    Point2D operator-(const Point2D& other) const { return {x - other.x, y - other.y}; }
    Point2D operator*(double s) const { return {x * s, y * s}; }

    double distanceTo(const Point2D& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

/// Bounding box
struct BoundingBox {
    Point2D min;
    Point2D max;

    bool isValid() const { return min.x <= max.x && min.y <= max.y; }
    bool contains(const Point2D& p) const {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
    }
    bool intersects(const BoundingBox& other) const {
        return !(other.max.x < min.x || other.min.x > max.x ||
                 other.max.y < min.y || other.min.y > max.y);
    }
    void expand(const Point2D& p) {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
    }
    void merge(const BoundingBox& other) {
        if (!other.isValid()) return;
        expand(other.min);
        expand(other.max);
    }
};

/// Entity types
enum class EntityType {
    Line,
    Circle,
    Arc,
    Polyline,
    Text,
    Insert,     // block reference
    Point
};

/// Line type for 도무송 classification
enum class LineClassification {
    Unknown,    // not classified
    Oshi,       // 오시선 (crease/fold line)
    Knife,      // 칼선 (cut line)
    Guide,      // 구아이선 (guide line)
    Bridge,     // 브릿지
    Outline     // 외곽선
};

using EntityId = uint64_t;

/// Base entity — all drawable objects derive from this
class Entity {
public:
    Entity(EntityType type, EntityId id);
    virtual ~Entity() = default;

    EntityId id() const { return m_id; }
    EntityType type() const { return m_type; }

    const std::string& layerName() const { return m_layerName; }
    void setLayerName(const std::string& name) { m_layerName = name; }

    int color() const { return m_color; }
    void setColor(int color) { m_color = color; }

    double lineWidth() const { return m_lineWidth; }
    void setLineWidth(double w) { m_lineWidth = w; }

    LineClassification classification() const { return m_classification; }
    void setClassification(LineClassification c) { m_classification = c; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }

    bool isSelected() const { return m_selected; }
    void setSelected(bool s) { m_selected = s; }

    /// Get bounding box for spatial indexing and viewport culling
    virtual BoundingBox boundingBox() const = 0;

    /// Clone this entity (deep copy)
    virtual std::unique_ptr<Entity> clone() const = 0;

    /// Move by delta
    virtual void translate(const Point2D& delta) = 0;

    /// Rotate around center by angle (degrees)
    virtual void rotate(const Point2D& center, double angleDeg) = 0;

    /// Mirror across a line defined by two points
    virtual void mirror(const Point2D& p1, const Point2D& p2) = 0;

    /// Scale from center by factor
    virtual void scale(const Point2D& center, double factor) = 0;

private:
    EntityId m_id;
    EntityType m_type;
    std::string m_layerName = "0";
    int m_color = 7; // white in AutoCAD convention
    double m_lineWidth = 0.0;
    LineClassification m_classification = LineClassification::Unknown;
    bool m_visible = true;
    bool m_selected = false;
};

/// Line entity — two endpoints
class LineEntity : public Entity {
public:
    LineEntity(EntityId id, const Point2D& start, const Point2D& end);

    const Point2D& start() const { return m_start; }
    const Point2D& end() const { return m_end; }
    void setStart(const Point2D& p) { m_start = p; }
    void setEnd(const Point2D& p) { m_end = p; }

    double length() const { return m_start.distanceTo(m_end); }

    BoundingBox boundingBox() const override;
    std::unique_ptr<Entity> clone() const override;
    void translate(const Point2D& delta) override;
    void rotate(const Point2D& center, double angleDeg) override;
    void mirror(const Point2D& p1, const Point2D& p2) override;
    void scale(const Point2D& center, double factor) override;

private:
    Point2D m_start;
    Point2D m_end;
};

/// Circle entity — center + radius
class CircleEntity : public Entity {
public:
    CircleEntity(EntityId id, const Point2D& center, double radius);

    const Point2D& center() const { return m_center; }
    double radius() const { return m_radius; }
    void setCenter(const Point2D& c) { m_center = c; }
    void setRadius(double r) { m_radius = r; }

    BoundingBox boundingBox() const override;
    std::unique_ptr<Entity> clone() const override;
    void translate(const Point2D& delta) override;
    void rotate(const Point2D& center, double angleDeg) override;
    void mirror(const Point2D& p1, const Point2D& p2) override;
    void scale(const Point2D& center, double factor) override;

private:
    Point2D m_center;
    double m_radius;
};

/// Arc entity — center + radius + start/end angle
class ArcEntity : public Entity {
public:
    ArcEntity(EntityId id, const Point2D& center, double radius,
              double startAngleDeg, double endAngleDeg);

    const Point2D& center() const { return m_center; }
    double radius() const { return m_radius; }
    double startAngle() const { return m_startAngle; }
    double endAngle() const { return m_endAngle; }

    BoundingBox boundingBox() const override;
    std::unique_ptr<Entity> clone() const override;
    void translate(const Point2D& delta) override;
    void rotate(const Point2D& center, double angleDeg) override;
    void mirror(const Point2D& p1, const Point2D& p2) override;
    void scale(const Point2D& center, double factor) override;

private:
    Point2D m_center;
    double m_radius;
    double m_startAngle; // degrees
    double m_endAngle;   // degrees
};

/// Polyline entity — ordered list of vertices
class PolylineEntity : public Entity {
public:
    PolylineEntity(EntityId id, const std::vector<Point2D>& vertices, bool closed = false);

    const std::vector<Point2D>& vertices() const { return m_vertices; }
    bool isClosed() const { return m_closed; }
    void setClosed(bool c) { m_closed = c; }
    void addVertex(const Point2D& p) { m_vertices.push_back(p); }

    BoundingBox boundingBox() const override;
    std::unique_ptr<Entity> clone() const override;
    void translate(const Point2D& delta) override;
    void rotate(const Point2D& center, double angleDeg) override;
    void mirror(const Point2D& p1, const Point2D& p2) override;
    void scale(const Point2D& center, double factor) override;

private:
    std::vector<Point2D> m_vertices;
    bool m_closed;
};

/// Text entity
class TextEntity : public Entity {
public:
    TextEntity(EntityId id, const Point2D& position, const std::string& text, double height);

    const Point2D& position() const { return m_position; }
    const std::string& text() const { return m_text; }
    double height() const { return m_height; }
    double rotation() const { return m_rotation; }
    void setPosition(const Point2D& p) { m_position = p; }
    void setText(const std::string& t) { m_text = t; }
    void setHeight(double h) { m_height = h; }
    void setRotation(double r) { m_rotation = r; }

    BoundingBox boundingBox() const override;
    std::unique_ptr<Entity> clone() const override;
    void translate(const Point2D& delta) override;
    void rotate(const Point2D& center, double angleDeg) override;
    void mirror(const Point2D& p1, const Point2D& p2) override;
    void scale(const Point2D& center, double factor) override;

private:
    Point2D m_position;
    std::string m_text;
    double m_height;
    double m_rotation = 0.0;
};

} // namespace yjcad
