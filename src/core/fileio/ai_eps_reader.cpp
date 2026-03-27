#include "ai_eps_reader.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <filesystem>

namespace yjcad {

AiEpsReader::AiEpsReader(Document& doc) : m_doc(doc) {}

AiEpsLoadResult AiEpsReader::load(const std::string& filePath) {
    AiEpsLoadResult result;

    if (!std::filesystem::exists(filePath)) {
        result.errorMessage = "File not found: " + filePath;
        return result;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        result.errorMessage = "Cannot open file: " + filePath;
        return result;
    }

    // Verify it's a PostScript file
    std::string firstLine;
    std::getline(file, firstLine);
    if (firstLine.find("%!PS") == std::string::npos &&
        firstLine.find("%!Adobe") == std::string::npos &&
        firstLine.find("%PDF") == std::string::npos) {
        result.errorMessage = "Not a valid AI/EPS file (missing PostScript header)";
        return result;
    }

    // Add import layer
    m_doc.layers().addLayer("ai_import", 5); // magenta

    // Reset state
    m_numStack.clear();
    m_currentPath.clear();
    m_currentPos = {0, 0};
    m_pathStart = {0, 0};
    m_inPath = false;
    m_entityCount = 0;
    m_pathCount = 0;

    // Parse line by line
    file.seekg(0);
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and binary sections
        if (line.empty() || line[0] == '%') continue;
        if (line.find("%%EndProlog") != std::string::npos) continue;
        if (line.find("%%Trailer") != std::string::npos) break;
        parseLine(line);
    }

    // Finish any open path
    if (!m_currentPath.empty()) {
        finishCurrentPath(false);
    }

    result.success = true;
    result.entityCount = m_entityCount;
    result.pathCount = m_pathCount;
    return result;
}

void AiEpsReader::parseLine(const std::string& line) {
    std::istringstream iss(line);
    std::string token;

    while (iss >> token) {
        // Try to parse as number
        char* end;
        double val = std::strtod(token.c_str(), &end);
        if (end != token.c_str() && *end == '\0') {
            m_numStack.push_back(val);
            continue;
        }

        // PostScript path operators
        if (token == "m" || token == "moveto") {
            // moveto: x y m
            if (m_numStack.size() >= 2) {
                // Finish previous path
                if (!m_currentPath.empty()) {
                    finishCurrentPath(false);
                }
                double y = m_numStack.back(); m_numStack.pop_back();
                double x = m_numStack.back(); m_numStack.pop_back();
                m_currentPos = {x, y};
                m_pathStart = m_currentPos;
                m_currentPath.push_back(m_currentPos);
                m_inPath = true;
            }
            m_numStack.clear();
        }
        else if (token == "l" || token == "L" || token == "lineto") {
            // lineto: x y l
            if (m_numStack.size() >= 2) {
                double y = m_numStack.back(); m_numStack.pop_back();
                double x = m_numStack.back(); m_numStack.pop_back();
                m_currentPos = {x, y};
                m_currentPath.push_back(m_currentPos);
            }
            m_numStack.clear();
        }
        else if (token == "c" || token == "C" || token == "curveto") {
            // curveto: x1 y1 x2 y2 x3 y3 c
            if (m_numStack.size() >= 6) {
                double y3 = m_numStack.back(); m_numStack.pop_back();
                double x3 = m_numStack.back(); m_numStack.pop_back();
                double y2 = m_numStack.back(); m_numStack.pop_back();
                double x2 = m_numStack.back(); m_numStack.pop_back();
                double y1 = m_numStack.back(); m_numStack.pop_back();
                double x1 = m_numStack.back(); m_numStack.pop_back();

                PathPoint p0 = m_currentPos;
                PathPoint p1 = {x1, y1};
                PathPoint p2 = {x2, y2};
                PathPoint p3 = {x3, y3};

                std::vector<PathPoint> bezierPts;
                flattenCubicBezier(p0, p1, p2, p3, bezierPts);
                for (const auto& pt : bezierPts) {
                    m_currentPath.push_back(pt);
                }
                m_currentPos = p3;
            }
            m_numStack.clear();
        }
        else if (token == "v" || token == "V") {
            // curveto variant: x2 y2 x3 y3 v (p1 = current point)
            if (m_numStack.size() >= 4) {
                double y3 = m_numStack.back(); m_numStack.pop_back();
                double x3 = m_numStack.back(); m_numStack.pop_back();
                double y2 = m_numStack.back(); m_numStack.pop_back();
                double x2 = m_numStack.back(); m_numStack.pop_back();

                PathPoint p0 = m_currentPos;
                PathPoint p1 = m_currentPos; // same as current
                PathPoint p2 = {x2, y2};
                PathPoint p3 = {x3, y3};

                std::vector<PathPoint> bezierPts;
                flattenCubicBezier(p0, p1, p2, p3, bezierPts);
                for (const auto& pt : bezierPts) {
                    m_currentPath.push_back(pt);
                }
                m_currentPos = p3;
            }
            m_numStack.clear();
        }
        else if (token == "y" || token == "Y") {
            // curveto variant: x1 y1 x3 y3 y (p2 = p3)
            if (m_numStack.size() >= 4) {
                double y3 = m_numStack.back(); m_numStack.pop_back();
                double x3 = m_numStack.back(); m_numStack.pop_back();
                double y1 = m_numStack.back(); m_numStack.pop_back();
                double x1 = m_numStack.back(); m_numStack.pop_back();

                PathPoint p0 = m_currentPos;
                PathPoint p1 = {x1, y1};
                PathPoint p2 = {x3, y3}; // same as end
                PathPoint p3 = {x3, y3};

                std::vector<PathPoint> bezierPts;
                flattenCubicBezier(p0, p1, p2, p3, bezierPts);
                for (const auto& pt : bezierPts) {
                    m_currentPath.push_back(pt);
                }
                m_currentPos = p3;
            }
            m_numStack.clear();
        }
        else if (token == "h" || token == "H" || token == "closepath") {
            // Close and finish
            finishCurrentPath(true);
            m_numStack.clear();
        }
        else if (token == "S" || token == "s" || token == "stroke" ||
                 token == "f" || token == "F" || token == "fill" ||
                 token == "B" || token == "b" || token == "n" || token == "N") {
            // End of path: stroke/fill/clip
            if (!m_currentPath.empty()) {
                bool closed = (token == "f" || token == "F" ||
                               token == "b" || token == "B");
                finishCurrentPath(closed);
            }
            m_numStack.clear();
        }
        else {
            // Unknown operator — clear number stack
            m_numStack.clear();
        }
    }
}

void AiEpsReader::finishCurrentPath(bool closed) {
    if (m_currentPath.size() < 2) {
        m_currentPath.clear();
        m_inPath = false;
        return;
    }

    // Convert to entity
    if (m_currentPath.size() == 2) {
        // Simple line
        EntityId id = m_doc.nextId();
        auto line = std::make_unique<LineEntity>(id,
            Point2D{m_currentPath[0].x, m_currentPath[0].y},
            Point2D{m_currentPath[1].x, m_currentPath[1].y});
        line->setLayerName("ai_import");
        auto cmd = std::make_unique<AddEntityCommand>(m_doc, std::move(line));
        m_doc.history().execute(std::move(cmd));
        ++m_entityCount;
    } else {
        // Polyline
        std::vector<Point2D> vertices;
        vertices.reserve(m_currentPath.size());
        for (const auto& pt : m_currentPath) {
            vertices.emplace_back(pt.x, pt.y);
        }
        EntityId id = m_doc.nextId();
        auto poly = std::make_unique<PolylineEntity>(id, vertices, closed);
        poly->setLayerName("ai_import");
        auto cmd = std::make_unique<AddEntityCommand>(m_doc, std::move(poly));
        m_doc.history().execute(std::move(cmd));
        ++m_entityCount;
    }

    ++m_pathCount;
    m_currentPath.clear();
    m_inPath = false;
}

void AiEpsReader::flattenCubicBezier(const PathPoint& p0, const PathPoint& p1,
                                      const PathPoint& p2, const PathPoint& p3,
                                      std::vector<PathPoint>& output) {
    // Adaptive subdivision: flatten until deviation < tolerance
    // Check if the curve is flat enough
    double dx = p3.x - p0.x;
    double dy = p3.y - p0.y;
    double lenSq = dx * dx + dy * dy;

    if (lenSq < 1e-12) {
        output.push_back(p3);
        return;
    }

    double len = std::sqrt(lenSq);
    // Distance of control points from the line p0→p3
    double d1 = std::abs((p1.x - p0.x) * dy - (p1.y - p0.y) * dx) / len;
    double d2 = std::abs((p2.x - p0.x) * dy - (p2.y - p0.y) * dx) / len;

    if (d1 + d2 <= m_flattenTol) {
        output.push_back(p3);
        return;
    }

    // Subdivide at t=0.5 (de Casteljau)
    PathPoint q0 = {(p0.x + p1.x) / 2, (p0.y + p1.y) / 2};
    PathPoint q1 = {(p1.x + p2.x) / 2, (p1.y + p2.y) / 2};
    PathPoint q2 = {(p2.x + p3.x) / 2, (p2.y + p3.y) / 2};
    PathPoint r0 = {(q0.x + q1.x) / 2, (q0.y + q1.y) / 2};
    PathPoint r1 = {(q1.x + q2.x) / 2, (q1.y + q2.y) / 2};
    PathPoint s  = {(r0.x + r1.x) / 2, (r0.y + r1.y) / 2};

    flattenCubicBezier(p0, q0, r0, s, output);
    flattenCubicBezier(s, r1, q2, p3, output);
}

} // namespace yjcad
