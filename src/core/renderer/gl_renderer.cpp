#include "gl_renderer.h"
#include <cmath>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;
static constexpr int ARC_SEGMENTS = 64; // segments per full circle

GlRenderer::GlRenderer() = default;

GlRenderer::~GlRenderer() {
    cleanup();
}

bool GlRenderer::initialize() {
    if (m_initialized) return true;

    initializeOpenGLFunctions();

    // Compile shaders (inline for simplicity — embedded strings)
    m_shader = std::make_unique<QOpenGLShaderProgram>();

    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec3 aColor;
        uniform mat4 uProjection;
        out vec3 vColor;
        void main() {
            gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
            vColor = aColor;
        }
    )";

    const char* fragSrc = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc)) {
        return false;
    }
    if (!m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc)) {
        return false;
    }
    if (!m_shader->link()) {
        return false;
    }

    m_projLoc = m_shader->uniformLocation("uProjection");

    // Entity VAO/VBO
    m_entityVao.create();
    m_entityVbo.create();

    // Grid VAO/VBO
    m_gridVao.create();
    m_gridVbo.create();

    m_initialized = true;
    return true;
}

void GlRenderer::getEntityColor(const Entity* entity, float& r, float& g, float& b) {
    if (entity->isSelected()) {
        r = EntityColors::SELECT_R;
        g = EntityColors::SELECT_G;
        b = EntityColors::SELECT_B;
        return;
    }

    switch (entity->classification()) {
        case LineClassification::Oshi:
            r = EntityColors::OSHI_R; g = EntityColors::OSHI_G; b = EntityColors::OSHI_B; break;
        case LineClassification::Knife:
            r = EntityColors::KNIFE_R; g = EntityColors::KNIFE_G; b = EntityColors::KNIFE_B; break;
        case LineClassification::Guide:
            r = EntityColors::GUIDE_R; g = EntityColors::GUIDE_G; b = EntityColors::GUIDE_B; break;
        case LineClassification::Bridge:
            r = EntityColors::BRIDGE_R; g = EntityColors::BRIDGE_G; b = EntityColors::BRIDGE_B; break;
        case LineClassification::Outline:
            r = EntityColors::OUTLINE_R; g = EntityColors::OUTLINE_G; b = EntityColors::OUTLINE_B; break;
        default:
            r = EntityColors::DEFAULT_R; g = EntityColors::DEFAULT_G; b = EntityColors::DEFAULT_B; break;
    }
}

void GlRenderer::tessellateCircle(const CircleEntity* circle, std::vector<GlVertex>& verts) {
    float r, g, b;
    getEntityColor(circle, r, g, b);
    double cx = circle->center().x;
    double cy = circle->center().y;
    double rad = circle->radius();

    for (int i = 0; i < ARC_SEGMENTS; ++i) {
        double a1 = 2.0 * PI * i / ARC_SEGMENTS;
        double a2 = 2.0 * PI * (i + 1) / ARC_SEGMENTS;
        verts.push_back({static_cast<float>(cx + rad * std::cos(a1)),
                         static_cast<float>(cy + rad * std::sin(a1)), r, g, b});
        verts.push_back({static_cast<float>(cx + rad * std::cos(a2)),
                         static_cast<float>(cy + rad * std::sin(a2)), r, g, b});
    }
}

void GlRenderer::tessellateArc(const ArcEntity* arc, std::vector<GlVertex>& verts) {
    float r, g, b;
    getEntityColor(arc, r, g, b);
    double cx = arc->center().x;
    double cy = arc->center().y;
    double rad = arc->radius();
    double startRad = arc->startAngle() * PI / 180.0;
    double endRad = arc->endAngle() * PI / 180.0;

    if (endRad <= startRad) endRad += 2.0 * PI;
    double span = endRad - startRad;
    int segments = std::max(4, static_cast<int>(ARC_SEGMENTS * span / (2.0 * PI)));

    for (int i = 0; i < segments; ++i) {
        double a1 = startRad + span * i / segments;
        double a2 = startRad + span * (i + 1) / segments;
        verts.push_back({static_cast<float>(cx + rad * std::cos(a1)),
                         static_cast<float>(cy + rad * std::sin(a1)), r, g, b});
        verts.push_back({static_cast<float>(cx + rad * std::cos(a2)),
                         static_cast<float>(cy + rad * std::sin(a2)), r, g, b});
    }
}

void GlRenderer::buildEntityVertices(const Document& doc) {
    m_entityVerts.clear();
    m_entityVerts.reserve(doc.entityCount() * 4);

    for (EntityId id : doc.entityIds()) {
        const Entity* entity = doc.entity(id);
        if (!entity || !entity->isVisible()) continue;

        float r, g, b;
        getEntityColor(entity, r, g, b);

        switch (entity->type()) {
            case EntityType::Line: {
                auto* line = static_cast<const LineEntity*>(entity);
                m_entityVerts.push_back({static_cast<float>(line->start().x),
                                         static_cast<float>(line->start().y), r, g, b});
                m_entityVerts.push_back({static_cast<float>(line->end().x),
                                         static_cast<float>(line->end().y), r, g, b});
                break;
            }
            case EntityType::Circle:
                tessellateCircle(static_cast<const CircleEntity*>(entity), m_entityVerts);
                break;
            case EntityType::Arc:
                tessellateArc(static_cast<const ArcEntity*>(entity), m_entityVerts);
                break;
            case EntityType::Polyline: {
                auto* poly = static_cast<const PolylineEntity*>(entity);
                const auto& verts = poly->vertices();
                for (size_t i = 0; i + 1 < verts.size(); ++i) {
                    m_entityVerts.push_back({static_cast<float>(verts[i].x),
                                             static_cast<float>(verts[i].y), r, g, b});
                    m_entityVerts.push_back({static_cast<float>(verts[i+1].x),
                                             static_cast<float>(verts[i+1].y), r, g, b});
                }
                if (poly->isClosed() && verts.size() > 1) {
                    m_entityVerts.push_back({static_cast<float>(verts.back().x),
                                             static_cast<float>(verts.back().y), r, g, b});
                    m_entityVerts.push_back({static_cast<float>(verts.front().x),
                                             static_cast<float>(verts.front().y), r, g, b});
                }
                break;
            }
            case EntityType::Text: {
                // Text: render as a small cross marker for now
                // Full text rendering requires FreeType (Phase 1 M2)
                auto* text = static_cast<const TextEntity*>(entity);
                float px = static_cast<float>(text->position().x);
                float py = static_cast<float>(text->position().y);
                float h = static_cast<float>(text->height());
                // Draw a small "T" marker
                m_entityVerts.push_back({px, py, r, g, b});
                m_entityVerts.push_back({px + h * 0.6f, py, r, g, b});
                m_entityVerts.push_back({px + h * 0.3f, py, r, g, b});
                m_entityVerts.push_back({px + h * 0.3f, py + h, r, g, b});
                break;
            }
            default:
                break;
        }
    }

    m_entityVertexCount = static_cast<int>(m_entityVerts.size());
}

void GlRenderer::rebuildFromDocument(const Document& doc) {
    if (!m_initialized) return;

    buildEntityVertices(doc);

    // Upload to GPU
    m_entityVao.bind();
    m_entityVbo.bind();
    m_entityVbo.allocate(m_entityVerts.data(),
                         static_cast<int>(m_entityVerts.size() * sizeof(GlVertex)));

    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GlVertex),
                          reinterpret_cast<void*>(0));

    // Color attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex),
                          reinterpret_cast<void*>(2 * sizeof(float)));

    m_entityVbo.release();
    m_entityVao.release();
}

void GlRenderer::buildGridVertices(const Camera& camera) {
    std::vector<GlVertex> gridVerts;
    auto bounds = camera.visibleBounds();

    // Adaptive grid spacing based on zoom
    double spacing = m_gridSpacing;
    double pixelSpacing = spacing * camera.zoom();
    while (pixelSpacing < 20.0) { spacing *= 5.0; pixelSpacing = spacing * camera.zoom(); }
    while (pixelSpacing > 200.0) { spacing /= 5.0; pixelSpacing = spacing * camera.zoom(); }

    float gr = EntityColors::GRID_R;
    float gg = EntityColors::GRID_G;
    float gb = EntityColors::GRID_B;

    double startX = std::floor(bounds.min.x / spacing) * spacing;
    double startY = std::floor(bounds.min.y / spacing) * spacing;

    // Vertical grid lines
    for (double x = startX; x <= bounds.max.x; x += spacing) {
        gridVerts.push_back({static_cast<float>(x), static_cast<float>(bounds.min.y), gr, gg, gb});
        gridVerts.push_back({static_cast<float>(x), static_cast<float>(bounds.max.y), gr, gg, gb});
    }
    // Horizontal grid lines
    for (double y = startY; y <= bounds.max.y; y += spacing) {
        gridVerts.push_back({static_cast<float>(bounds.min.x), static_cast<float>(y), gr, gg, gb});
        gridVerts.push_back({static_cast<float>(bounds.max.x), static_cast<float>(y), gr, gg, gb});
    }

    // Origin cross (brighter)
    float ox = 0.4f;
    gridVerts.push_back({static_cast<float>(bounds.min.x), 0.0f, ox, 0.0f, 0.0f});
    gridVerts.push_back({static_cast<float>(bounds.max.x), 0.0f, ox, 0.0f, 0.0f});
    gridVerts.push_back({0.0f, static_cast<float>(bounds.min.y), 0.0f, ox, 0.0f});
    gridVerts.push_back({0.0f, static_cast<float>(bounds.max.y), 0.0f, ox, 0.0f});

    m_gridVertexCount = static_cast<int>(gridVerts.size());

    m_gridVao.bind();
    m_gridVbo.bind();
    m_gridVbo.allocate(gridVerts.data(),
                       static_cast<int>(gridVerts.size() * sizeof(GlVertex)));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GlVertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex),
                          reinterpret_cast<void*>(2 * sizeof(float)));

    m_gridVbo.release();
    m_gridVao.release();
}

void GlRenderer::render(const Camera& camera) {
    if (!m_initialized) return;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background (Dark mode)
    glClear(GL_COLOR_BUFFER_BIT);

    m_shader->bind();
    m_shader->setUniformValue(m_projLoc, camera.projectionMatrix());

    // Draw grid first (behind entities)
    if (m_gridEnabled) {
        buildGridVertices(camera);
        m_gridVao.bind();
        glDrawArrays(GL_LINES, 0, m_gridVertexCount);
        m_gridVao.release();
    }

    // Draw entities
    if (m_entityVertexCount > 0) {
        m_entityVao.bind();
        glDrawArrays(GL_LINES, 0, m_entityVertexCount);
        m_entityVao.release();
    }

    m_shader->release();
}

void GlRenderer::cleanup() {
    if (!m_initialized) return;
    m_entityVbo.destroy();
    m_entityVao.destroy();
    m_gridVbo.destroy();
    m_gridVao.destroy();
    m_shader.reset();
    m_initialized = false;
}

} // namespace yjcad
