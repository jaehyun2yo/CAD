#include "operations.h"
#include <clipper2/clipper.h>
#include <cmath>
#include <algorithm>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;
// Clipper2 uses integer coordinates — scale factor for mm precision
static constexpr double CLIPPER_SCALE = 1000.0;

std::unique_ptr<PolylineEntity> GeometryOps::offsetPolyline(
    const PolylineEntity& poly, double distance, EntityId newId) {

    if (poly.vertices().size() < 2) return nullptr;

    // Convert to Clipper2 path
    Clipper2Lib::PathD path;
    for (const auto& v : poly.vertices()) {
        path.push_back({v.x, v.y});
    }

    Clipper2Lib::PathsD result;
    if (poly.isClosed()) {
        result = Clipper2Lib::InflatePaths({path}, distance,
            Clipper2Lib::JoinType::Miter,
            Clipper2Lib::EndType::Polygon);
    } else {
        result = Clipper2Lib::InflatePaths({path}, distance,
            Clipper2Lib::JoinType::Miter,
            Clipper2Lib::EndType::Butt);
    }

    if (result.empty() || result[0].empty()) return nullptr;

    // Convert back to YJ-CAD polyline
    std::vector<Point2D> newVerts;
    newVerts.reserve(result[0].size());
    for (const auto& pt : result[0]) {
        newVerts.emplace_back(pt.x, pt.y);
    }

    auto newPoly = std::make_unique<PolylineEntity>(newId, newVerts, poly.isClosed());
    newPoly->setLayerName(poly.layerName());
    newPoly->setColor(poly.color());
    newPoly->setClassification(poly.classification());
    return newPoly;
}

std::vector<std::unique_ptr<LineEntity>> GeometryOps::insertBridges(
    const LineEntity& line, double bridgeWidth, double spacing, EntityId startId) {

    std::vector<std::unique_ptr<LineEntity>> segments;

    double totalLen = line.length();
    if (totalLen < spacing || bridgeWidth <= 0 || spacing <= 0) {
        // Line too short for bridges — return as-is
        segments.push_back(std::make_unique<LineEntity>(startId, line.start(), line.end()));
        segments.back()->setClassification(line.classification());
        segments.back()->setLayerName(line.layerName());
        return segments;
    }

    Point2D dir{
        (line.end().x - line.start().x) / totalLen,
        (line.end().y - line.start().y) / totalLen
    };

    double pos = 0.0;
    EntityId id = startId;
    bool drawingLine = true;

    // Pattern: [line segment] [bridge gap] [line segment] [bridge gap] ...
    // Start with half-spacing offset
    double firstSegLen = spacing / 2.0;
    pos = 0.0;

    while (pos < totalLen) {
        double segEnd;
        if (drawingLine) {
            segEnd = std::min(pos + (pos == 0.0 ? firstSegLen : spacing), totalLen);
            // Create line segment
            Point2D p1{line.start().x + dir.x * pos, line.start().y + dir.y * pos};
            Point2D p2{line.start().x + dir.x * segEnd, line.start().y + dir.y * segEnd};
            auto seg = std::make_unique<LineEntity>(id++, p1, p2);
            seg->setClassification(line.classification());
            seg->setLayerName(line.layerName());
            seg->setColor(line.color());
            segments.push_back(std::move(seg));
            pos = segEnd;
        } else {
            // Bridge gap
            pos = std::min(pos + bridgeWidth, totalLen);
        }
        drawingLine = !drawingLine;
    }

    return segments;
}

std::unique_ptr<PolylineEntity> GeometryOps::arcToPolyline(
    const ArcEntity& arc, double tolerance, EntityId newId) {

    double startRad = arc.startAngle() * PI / 180.0;
    double endRad = arc.endAngle() * PI / 180.0;
    if (endRad <= startRad) endRad += 2.0 * PI;
    double span = endRad - startRad;

    // Calculate number of segments from tolerance
    // For arc: segments = ceil(span / (2 * acos(1 - tolerance / radius)))
    double radius = arc.radius();
    int segments;
    if (tolerance >= radius) {
        segments = 4;
    } else {
        double anglePerSeg = 2.0 * std::acos(1.0 - tolerance / radius);
        segments = std::max(4, static_cast<int>(std::ceil(span / anglePerSeg)));
    }

    std::vector<Point2D> vertices;
    vertices.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        double a = startRad + span * i / segments;
        vertices.emplace_back(
            arc.center().x + radius * std::cos(a),
            arc.center().y + radius * std::sin(a)
        );
    }

    auto poly = std::make_unique<PolylineEntity>(newId, vertices, false);
    poly->setLayerName(arc.layerName());
    poly->setColor(arc.color());
    poly->setClassification(arc.classification());
    return poly;
}

std::unique_ptr<PolylineEntity> GeometryOps::circleToPolyline(
    const CircleEntity& circle, int segments, EntityId newId) {

    std::vector<Point2D> vertices;
    vertices.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        double a = 2.0 * PI * i / segments;
        vertices.emplace_back(
            circle.center().x + circle.radius() * std::cos(a),
            circle.center().y + circle.radius() * std::sin(a)
        );
    }

    auto poly = std::make_unique<PolylineEntity>(newId, vertices, true);
    poly->setLayerName(circle.layerName());
    poly->setColor(circle.color());
    poly->setClassification(circle.classification());
    return poly;
}

double GeometryOps::pointToSegmentDistance(const Point2D& p,
                                            const Point2D& a, const Point2D& b) {
    double dx = b.x - a.x, dy = b.y - a.y;
    double lenSq = dx * dx + dy * dy;
    if (lenSq < 1e-18) return p.distanceTo(a);
    double t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / lenSq, 0.0, 1.0);
    Point2D proj{a.x + t * dx, a.y + t * dy};
    return p.distanceTo(proj);
}

} // namespace yjcad
