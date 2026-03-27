#pragma once

#include "camera.h"
#include "core/document/document.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <vector>
#include <memory>

namespace yjcad {

/// Vertex for the GPU: position (x,y) + color (r,g,b)
struct GlVertex {
    float x, y;
    float r, g, b;
};

/// Color mapping for entity classification / selection
struct EntityColors {
    static constexpr float KNIFE_R = 1.0f, KNIFE_G = 1.0f, KNIFE_B = 1.0f;     // white
    static constexpr float OSHI_R = 1.0f, OSHI_G = 0.0f, OSHI_B = 0.0f;         // red
    static constexpr float GUIDE_R = 0.0f, GUIDE_G = 1.0f, GUIDE_B = 0.0f;       // green
    static constexpr float BRIDGE_R = 1.0f, BRIDGE_G = 1.0f, BRIDGE_B = 0.0f;    // yellow
    static constexpr float OUTLINE_R = 0.0f, OUTLINE_G = 0.5f, OUTLINE_B = 1.0f; // blue
    static constexpr float DEFAULT_R = 0.8f, DEFAULT_G = 0.8f, DEFAULT_B = 0.8f; // light gray
    static constexpr float SELECT_R = 0.0f, SELECT_G = 1.0f, SELECT_B = 1.0f;    // cyan
    static constexpr float GRID_R = 0.2f, GRID_G = 0.2f, GRID_B = 0.2f;          // dark gray
};

/// OpenGL Renderer — renders Document entities using VBOs
class GlRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    GlRenderer();
    ~GlRenderer();

    /// Initialize OpenGL resources (call once after context is current)
    bool initialize();

    /// Rebuild VBOs from document entities (call when entities change)
    void rebuildFromDocument(const Document& doc);

    /// Render all entities + grid
    void render(const Camera& camera);

    /// Cleanup OpenGL resources
    void cleanup();

    bool isInitialized() const { return m_initialized; }

    // ── Grid ──
    void setGridEnabled(bool enabled) { m_gridEnabled = enabled; }
    void setGridSpacing(double spacing) { m_gridSpacing = spacing; }

    // ── Background ──
    void setBackgroundColor(float r, float g, float b) {
        m_bgR = r; m_bgG = g; m_bgB = b;
    }

private:
    void buildEntityVertices(const Document& doc);
    void buildGridVertices(const Camera& camera);
    void getEntityColor(const Entity* entity, float& r, float& g, float& b);

    // Tessellate curved entities into line segments
    void tessellateCircle(const CircleEntity* circle, std::vector<GlVertex>& verts);
    void tessellateArc(const ArcEntity* arc, std::vector<GlVertex>& verts);

    bool m_initialized = false;

    // Shader
    std::unique_ptr<QOpenGLShaderProgram> m_shader;
    int m_projLoc = -1;

    // Entity geometry (static VBO)
    QOpenGLVertexArrayObject m_entityVao;
    QOpenGLBuffer m_entityVbo{QOpenGLBuffer::VertexBuffer};
    int m_entityVertexCount = 0;

    // Grid geometry (rebuilt on zoom/pan)
    QOpenGLVertexArrayObject m_gridVao;
    QOpenGLBuffer m_gridVbo{QOpenGLBuffer::VertexBuffer};
    int m_gridVertexCount = 0;

    // Grid settings
    bool m_gridEnabled = true;
    double m_gridSpacing = 10.0;

    // Preview geometry (rubber-band lines from tools)
    QOpenGLVertexArrayObject m_previewVao;
    QOpenGLBuffer m_previewVbo{QOpenGLBuffer::VertexBuffer};
    int m_previewVertexCount = 0;

public:
    /// Update preview lines (tool rubber-band, snap indicator)
    void updatePreview(const std::vector<std::pair<Point2D, Point2D>>& lines,
                       const Point2D* snapPoint = nullptr);

private:
    // Cached entity vertices for partial updates
    std::vector<GlVertex> m_entityVerts;

    // Background color
    float m_bgR = 0.0f, m_bgG = 0.0f, m_bgB = 0.0f;
};

} // namespace yjcad
