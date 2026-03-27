#pragma once

#include "core/document/entity.h"
#include "core/document/document.h"
#include <vector>
#include <optional>

namespace yjcad {

enum class SnapType {
    None,
    Grid,
    Endpoint,
    Midpoint,
    Center,
    Nearest,
    Intersection,
    Perpendicular,
    Tangent,
    Quadrant,
    Extension
};

struct SnapResult {
    Point2D point;
    SnapType type = SnapType::None;
    bool valid = false;
};

/// Snap engine — finds nearest snap point for precise drawing
class SnapEngine {
public:
    explicit SnapEngine(const Document& doc);

    /// Enable/disable snap types
    void setGridSnap(bool enabled, double spacing = 1.0);
    void setEndpointSnap(bool enabled)     { m_endpointSnap = enabled; }
    void setMidpointSnap(bool enabled)     { m_midpointSnap = enabled; }
    void setCenterSnap(bool enabled)       { m_centerSnap = enabled; }
    void setNearestSnap(bool enabled)      { m_nearestSnap = enabled; }
    void setIntersectionSnap(bool enabled) { m_intersectionSnap = enabled; }
    void setPerpendicularSnap(bool enabled){ m_perpendicularSnap = enabled; }
    void setTangentSnap(bool enabled)      { m_tangentSnap = enabled; }
    void setQuadrantSnap(bool enabled)     { m_quadrantSnap = enabled; }

    /// Optional reference point used for Perpendicular/Tangent snaps
    /// (set to the last committed point while drawing)
    void setReferencePoint(const Point2D& p) { m_refPoint = p; m_hasRef = true; }
    void clearReferencePoint()               { m_hasRef = false; }

    /// Find best snap point near worldPos within tolerance
    /// Priority: Endpoint > Intersection > Midpoint > Perpendicular > Tangent
    ///           > Quadrant > Center > Nearest > Grid
    SnapResult snap(const Point2D& worldPos, double tolerance) const;

private:
    SnapResult snapToGrid(const Point2D& pos) const;
    SnapResult snapToEndpoints(const Point2D& pos, double tol) const;
    SnapResult snapToMidpoints(const Point2D& pos, double tol) const;
    SnapResult snapToCenters(const Point2D& pos, double tol) const;
    SnapResult snapToNearest(const Point2D& pos, double tol) const;
    SnapResult snapToIntersections(const Point2D& pos, double tol) const;
    SnapResult snapToPerpendicular(const Point2D& pos, double tol) const;
    SnapResult snapToTangent(const Point2D& pos, double tol) const;
    SnapResult snapToQuadrant(const Point2D& pos, double tol) const;

    const Document& m_doc;
    bool m_gridSnap        = true;
    double m_gridSpacing   = 1.0;
    bool m_endpointSnap    = true;
    bool m_midpointSnap    = true;
    bool m_centerSnap      = true;
    bool m_nearestSnap     = true;
    bool m_intersectionSnap   = true;
    bool m_perpendicularSnap  = true;
    bool m_tangentSnap        = true;
    bool m_quadrantSnap       = true;

    Point2D m_refPoint;
    bool    m_hasRef = false;
};

} // namespace yjcad
