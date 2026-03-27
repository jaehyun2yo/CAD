#include "geometry_3d.h"
#include <set>
#include <algorithm>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;

// ── Mesh3D ──

std::vector<std::pair<int, int>> Mesh3D::wireframeEdges() const {
    std::set<std::pair<int, int>> edgeSet;
    for (const auto& f : m_faces) {
        auto addEdge = [&](int a, int b) {
            edgeSet.insert({std::min(a, b), std::max(a, b)});
        };
        addEdge(f.v0, f.v1);
        addEdge(f.v1, f.v2);
        addEdge(f.v2, f.v0);
    }
    return {edgeSet.begin(), edgeSet.end()};
}

Point3D Mesh3D::faceNormal(int faceIdx) const {
    if (faceIdx < 0 || faceIdx >= static_cast<int>(m_faces.size()))
        return {0, 0, 1};

    const auto& f = m_faces[faceIdx];
    Point3D e1 = m_vertices[f.v1] - m_vertices[f.v0];
    Point3D e2 = m_vertices[f.v2] - m_vertices[f.v0];
    return Point3D::cross(e1, e2).normalized();
}

void Mesh3D::translate(const Point3D& delta) {
    for (auto& v : m_vertices) {
        v.x += delta.x;
        v.y += delta.y;
        v.z += delta.z;
    }
}

void Mesh3D::scale(const Point3D& center, double factor) {
    for (auto& v : m_vertices) {
        v.x = center.x + (v.x - center.x) * factor;
        v.y = center.y + (v.y - center.y) * factor;
        v.z = center.z + (v.z - center.z) * factor;
    }
}

// ── GeometryOps3D ──

std::unique_ptr<Mesh3D> GeometryOps3D::createBox(const Point3D& min, const Point3D& max) {
    auto mesh = std::make_unique<Mesh3D>();

    // 8 vertices
    mesh->addVertex({min.x, min.y, min.z}); // 0
    mesh->addVertex({max.x, min.y, min.z}); // 1
    mesh->addVertex({max.x, max.y, min.z}); // 2
    mesh->addVertex({min.x, max.y, min.z}); // 3
    mesh->addVertex({min.x, min.y, max.z}); // 4
    mesh->addVertex({max.x, min.y, max.z}); // 5
    mesh->addVertex({max.x, max.y, max.z}); // 6
    mesh->addVertex({min.x, max.y, max.z}); // 7

    // 12 triangles (2 per face)
    // Bottom (z=min)
    mesh->addFace(0, 1, 2); mesh->addFace(0, 2, 3);
    // Top (z=max)
    mesh->addFace(4, 6, 5); mesh->addFace(4, 7, 6);
    // Front (y=min)
    mesh->addFace(0, 5, 1); mesh->addFace(0, 4, 5);
    // Back (y=max)
    mesh->addFace(2, 7, 3); mesh->addFace(2, 6, 7);
    // Left (x=min)
    mesh->addFace(0, 3, 7); mesh->addFace(0, 7, 4);
    // Right (x=max)
    mesh->addFace(1, 5, 6); mesh->addFace(1, 6, 2);

    return mesh;
}

std::unique_ptr<Mesh3D> GeometryOps3D::createCylinder(const Point3D& center,
                                                        double radius, double height,
                                                        int segments) {
    auto mesh = std::make_unique<Mesh3D>();

    // Bottom center
    int bottomCenter = 0;
    mesh->addVertex({center.x, center.y, center.z});

    // Bottom ring
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * PI * i / segments;
        mesh->addVertex({center.x + radius * std::cos(angle),
                         center.y + radius * std::sin(angle),
                         center.z});
    }

    // Top center
    int topCenter = segments + 1;
    mesh->addVertex({center.x, center.y, center.z + height});

    // Top ring
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * PI * i / segments;
        mesh->addVertex({center.x + radius * std::cos(angle),
                         center.y + radius * std::sin(angle),
                         center.z + height});
    }

    // Bottom cap triangles
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        mesh->addFace(bottomCenter, i + 1, next + 1);
    }

    // Top cap triangles
    int topRingStart = topCenter + 1;
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        mesh->addFace(topCenter, topRingStart + next, topRingStart + i);
    }

    // Side triangles
    int bottomRingStart = 1;
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        int b0 = bottomRingStart + i;
        int b1 = bottomRingStart + next;
        int t0 = topRingStart + i;
        int t1 = topRingStart + next;
        mesh->addFace(b0, b1, t1);
        mesh->addFace(b0, t1, t0);
    }

    return mesh;
}

std::unique_ptr<Mesh3D> GeometryOps3D::extrudePolyline(const PolylineEntity& profile,
                                                        double height) {
    return extrudePolyline(profile, {0, 0, height});
}

std::unique_ptr<Mesh3D> GeometryOps3D::extrudePolyline(const PolylineEntity& profile,
                                                        const Point3D& direction) {
    auto mesh = std::make_unique<Mesh3D>();
    const auto& verts = profile.vertices();
    if (verts.size() < 2) return mesh;

    int n = static_cast<int>(verts.size());

    // Bottom profile (z=0)
    for (const auto& v : verts) {
        mesh->addVertex({v.x, v.y, 0});
    }

    // Top profile (translated by direction)
    for (const auto& v : verts) {
        mesh->addVertex({v.x + direction.x, v.y + direction.y, direction.z});
    }

    // Side faces
    int limit = profile.isClosed() ? n : n - 1;
    for (int i = 0; i < limit; ++i) {
        int next = (i + 1) % n;
        int b0 = i, b1 = next;
        int t0 = n + i, t1 = n + next;
        mesh->addFace(b0, b1, t1);
        mesh->addFace(b0, t1, t0);
    }

    // Cap faces (only for closed profiles, using simple fan triangulation)
    if (profile.isClosed() && n >= 3) {
        // Bottom cap
        for (int i = 1; i < n - 1; ++i) {
            mesh->addFace(0, i, i + 1);
        }
        // Top cap
        for (int i = 1; i < n - 1; ++i) {
            mesh->addFace(n, n + i + 1, n + i);
        }
    }

    return mesh;
}

} // namespace yjcad
