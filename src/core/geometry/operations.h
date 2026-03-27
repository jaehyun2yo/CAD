#pragma once

#include "core/document/entity.h"
#include <vector>
#include <memory>

namespace yjcad {

/// Geometry operations for 도무송 processing
class GeometryOps {
public:
    /// Offset a polyline by distance (positive = outward, negative = inward)
    /// Returns new polyline entity or nullptr on failure
    static std::unique_ptr<PolylineEntity> offsetPolyline(
        const PolylineEntity& poly, double distance, EntityId newId);

    /// Insert bridges into a line at regular intervals
    /// Returns a list of line segments with gaps (bridges)
    /// bridgeWidth: width of each bridge gap (mm)
    /// spacing: distance between bridges (mm)
    static std::vector<std::unique_ptr<LineEntity>> insertBridges(
        const LineEntity& line, double bridgeWidth, double spacing, EntityId startId);

    /// Convert arc to line segments (점빼기 / arc-to-lines)
    /// tolerance: max deviation from arc (mm)
    static std::unique_ptr<PolylineEntity> arcToPolyline(
        const ArcEntity& arc, double tolerance, EntityId newId);

    /// Convert circle to polyline
    static std::unique_ptr<PolylineEntity> circleToPolyline(
        const CircleEntity& circle, int segments, EntityId newId);

    /// Point-to-line-segment distance
    static double pointToSegmentDistance(const Point2D& p,
                                         const Point2D& a, const Point2D& b);

    /// Line-line intersection (segments)
    /// Returns true if segments intersect, point is set to intersection
    static bool lineLineIntersection(const Point2D& a1, const Point2D& a2,
                                     const Point2D& b1, const Point2D& b2,
                                     Point2D& intersection);

    /// Line-circle intersection
    /// Returns number of intersections (0, 1, or 2)
    static int lineCircleIntersection(const Point2D& lineStart, const Point2D& lineEnd,
                                      const Point2D& center, double radius,
                                      Point2D& hit1, Point2D& hit2);

    /// Split a line entity at a point, returns two new lines (or one if point is at endpoint)
    static std::vector<std::unique_ptr<LineEntity>> splitLineAt(
        const LineEntity& line, const Point2D& splitPoint, EntityId startId);
};

} // namespace yjcad
