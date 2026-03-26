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
    Nearest
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
    void setEndpointSnap(bool enabled) { m_endpointSnap = enabled; }
    void setMidpointSnap(bool enabled) { m_midpointSnap = enabled; }
    void setCenterSnap(bool enabled) { m_centerSnap = enabled; }

    /// Find best snap point near worldPos within tolerance
    SnapResult snap(const Point2D& worldPos, double tolerance) const;

private:
    SnapResult snapToGrid(const Point2D& pos) const;
    SnapResult snapToEndpoints(const Point2D& pos, double tol) const;
    SnapResult snapToMidpoints(const Point2D& pos, double tol) const;
    SnapResult snapToCenters(const Point2D& pos, double tol) const;

    const Document& m_doc;
    bool m_gridSnap = true;
    double m_gridSpacing = 1.0;
    bool m_endpointSnap = true;
    bool m_midpointSnap = true;
    bool m_centerSnap = true;
};

} // namespace yjcad
