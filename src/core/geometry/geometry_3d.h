#pragma once

#include "core/document/entity.h"
#include <vector>
#include <memory>
#include <cmath>

namespace yjcad {

/// 3D point
struct Point3D {
    double x = 0.0, y = 0.0, z = 0.0;

    Point3D() = default;
    Point3D(double x, double y, double z) : x(x), y(y), z(z) {}

    Point3D operator+(const Point3D& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Point3D operator-(const Point3D& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Point3D operator*(double s) const { return {x * s, y * s, z * s}; }

    double length() const { return std::sqrt(x * x + y * y + z * z); }

    Point3D normalized() const {
        double len = length();
        return (len > 1e-12) ? Point3D{x / len, y / len, z / len} : Point3D{0, 0, 0};
    }

    static Point3D cross(const Point3D& a, const Point3D& b) {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    static double dot(const Point3D& a, const Point3D& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
};

/// Triangle face (3 vertex indices)
struct Face3D {
    int v0, v1, v2;
};

/// 3D Mesh — triangle mesh geometry
class Mesh3D {
public:
    Mesh3D() = default;

    void addVertex(const Point3D& p) { m_vertices.push_back(p); }
    void addFace(int v0, int v1, int v2) { m_faces.push_back({v0, v1, v2}); }

    const std::vector<Point3D>& vertices() const { return m_vertices; }
    const std::vector<Face3D>& faces() const { return m_faces; }

    size_t vertexCount() const { return m_vertices.size(); }
    size_t faceCount() const { return m_faces.size(); }

    /// Get wireframe edges (pairs of vertex indices, deduplicated)
    std::vector<std::pair<int, int>> wireframeEdges() const;

    /// Compute face normal
    Point3D faceNormal(int faceIdx) const;

    /// Transform all vertices
    void translate(const Point3D& delta);
    void scale(const Point3D& center, double factor);

private:
    std::vector<Point3D> m_vertices;
    std::vector<Face3D> m_faces;
};

/// 3D Geometry Operations — basic solid modeling without OpenCASCADE
class GeometryOps3D {
public:
    /// Create a box mesh
    static std::unique_ptr<Mesh3D> createBox(const Point3D& min, const Point3D& max);

    /// Create a cylinder mesh
    static std::unique_ptr<Mesh3D> createCylinder(const Point3D& center,
                                                    double radius, double height,
                                                    int segments = 24);

    /// Extrude a 2D polyline along Z axis to create a 3D mesh
    /// The polyline defines the profile in the XY plane
    static std::unique_ptr<Mesh3D> extrudePolyline(const PolylineEntity& profile,
                                                    double height);

    /// Extrude a 2D polyline along an arbitrary direction
    static std::unique_ptr<Mesh3D> extrudePolyline(const PolylineEntity& profile,
                                                    const Point3D& direction);
};

} // namespace yjcad
