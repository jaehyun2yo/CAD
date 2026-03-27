#pragma once

#include "core/document/document.h"
#include <string>
#include <vector>

namespace yjcad {

/// Result of an AI/EPS file import
struct AiEpsLoadResult {
    bool success = false;
    std::string errorMessage;
    size_t entityCount = 0;
    size_t pathCount = 0;
};

/// AI/EPS Reader — imports Adobe Illustrator and EPS vector paths
/// Parses PostScript path operators directly (no Ghostscript needed).
/// Supports: moveto, lineto, curveto, closepath
/// Bezier curves are flattened to polylines.
class AiEpsReader {
public:
    explicit AiEpsReader(Document& doc);

    /// Import an AI or EPS file
    AiEpsLoadResult load(const std::string& filePath);

    /// Set bezier curve flattening tolerance (default: 0.5mm)
    void setFlattenTolerance(double tol) { m_flattenTol = tol; }

private:
    struct PathPoint {
        double x, y;
    };

    void parseLine(const std::string& line);
    void finishCurrentPath(bool closed);
    void flattenCubicBezier(const PathPoint& p0, const PathPoint& p1,
                             const PathPoint& p2, const PathPoint& p3,
                             std::vector<PathPoint>& output);

    Document& m_doc;
    double m_flattenTol = 0.5;

    // Parser state
    std::vector<double> m_numStack;
    std::vector<PathPoint> m_currentPath;
    PathPoint m_currentPos{0, 0};
    PathPoint m_pathStart{0, 0};
    bool m_inPath = false;
    size_t m_entityCount = 0;
    size_t m_pathCount = 0;
};

} // namespace yjcad
