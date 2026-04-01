#include "gl_renderer.h"
#include <cmath>

namespace yjcad {

static constexpr double PI = 3.14159265358979323846;
static constexpr int ARC_SEGMENTS = 64; // segments per full circle

// Standard AutoCAD Color Index (ACI) table — 256 entries as {R,G,B} (0-255)
static constexpr unsigned char ACI_TABLE[256][3] = {
    {  0,  0,  0}, // 0: BYBLOCK (use white as fallback)
    {255,  0,  0}, // 1: Red
    {255,255,  0}, // 2: Yellow
    {  0,255,  0}, // 3: Green
    {  0,255,255}, // 4: Cyan
    {  0,  0,255}, // 5: Blue
    {255,  0,255}, // 6: Magenta
    {255,255,255}, // 7: White
    {128,128,128}, // 8: Dark gray
    {192,192,192}, // 9: Light gray
    {255,  0,  0}, // 10
    {255,127,127}, // 11
    {165,  0,  0}, // 12
    {165, 82, 82}, // 13
    {127,  0,  0}, // 14
    {127, 63, 63}, // 15
    {76,   0,  0}, // 16
    {76,  38, 38}, // 17
    {38,   0,  0}, // 18
    {38,  19, 19}, // 19
    {255, 63,  0}, // 20
    {255,159,127}, // 21
    {165, 41,  0}, // 22
    {165,103, 82}, // 23
    {127, 31,  0}, // 24
    {127, 79, 63}, // 25
    {76,  19,  0}, // 26
    {76,  47, 38}, // 27
    {38,   9,  0}, // 28
    {38,  23, 19}, // 29
    {255,127,  0}, // 30
    {255,191,127}, // 31
    {165, 82,  0}, // 32
    {165,124, 82}, // 33
    {127, 63,  0}, // 34
    {127, 95, 63}, // 35
    {76,  38,  0}, // 36
    {76,  57, 38}, // 37
    {38,  19,  0}, // 38
    {38,  28, 19}, // 39
    {255,191,  0}, // 40
    {255,223,127}, // 41
    {165,124,  0}, // 42
    {165,145, 82}, // 43
    {127, 95,  0}, // 44
    {127,111, 63}, // 45
    {76,  57,  0}, // 46
    {76,  66, 38}, // 47
    {38,  28,  0}, // 48
    {38,  33, 19}, // 49
    {255,255,  0}, // 50
    {255,255,127}, // 51
    {165,165,  0}, // 52
    {165,165, 82}, // 53
    {127,127,  0}, // 54
    {127,127, 63}, // 55
    {76,  76,  0}, // 56
    {76,  76, 38}, // 57
    {38,  38,  0}, // 58
    {38,  38, 19}, // 59
    {191,255,  0}, // 60
    {223,255,127}, // 61
    {124,165,  0}, // 62
    {145,165, 82}, // 63
    {95, 127,  0}, // 64
    {111,127, 63}, // 65
    {57,  76,  0}, // 66
    {66,  76, 38}, // 67
    {28,  38,  0}, // 68
    {33,  38, 19}, // 69
    {127,255,  0}, // 70
    {191,255,127}, // 71
    {82, 165,  0}, // 72
    {124,165, 82}, // 73
    {63, 127,  0}, // 74
    {95, 127, 63}, // 75
    {38,  76,  0}, // 76
    {57,  76, 38}, // 77
    {19,  38,  0}, // 78
    {28,  38, 19}, // 79
    {63, 255,  0}, // 80
    {159,255,127}, // 81
    {41, 165,  0}, // 82
    {103,165, 82}, // 83
    {31, 127,  0}, // 84
    {79, 127, 63}, // 85
    {19,  76,  0}, // 86
    {47,  76, 38}, // 87
    {9,   38,  0}, // 88
    {23,  38, 19}, // 89
    {  0,255,  0}, // 90
    {127,255,127}, // 91
    {  0,165,  0}, // 92
    {82, 165, 82}, // 93
    {  0,127,  0}, // 94
    {63, 127, 63}, // 95
    {  0, 76,  0}, // 96
    {38,  76, 38}, // 97
    {  0, 38,  0}, // 98
    {19,  38, 19}, // 99
    {  0,255, 63}, // 100
    {127,255,159}, // 101
    {  0,165, 41}, // 102
    {82, 165,103}, // 103
    {  0,127, 31}, // 104
    {63, 127, 79}, // 105
    {  0, 76, 19}, // 106
    {38,  76, 47}, // 107
    {  0, 38,  9}, // 108
    {19,  38, 23}, // 109
    {  0,255,127}, // 110
    {127,255,191}, // 111
    {  0,165, 82}, // 112
    {82, 165,124}, // 113
    {  0,127, 63}, // 114
    {63, 127, 95}, // 115
    {  0, 76, 38}, // 116
    {38,  76, 57}, // 117
    {  0, 38, 19}, // 118
    {19,  38, 28}, // 119
    {  0,255,191}, // 120
    {127,255,223}, // 121
    {  0,165,124}, // 122
    {82, 165,145}, // 123
    {  0,127, 95}, // 124
    {63, 127,111}, // 125
    {  0, 76, 57}, // 126
    {38,  76, 66}, // 127
    {  0, 38, 28}, // 128
    {19,  38, 33}, // 129
    {  0,255,255}, // 130
    {127,255,255}, // 131
    {  0,165,165}, // 132
    {82, 165,165}, // 133
    {  0,127,127}, // 134
    {63, 127,127}, // 135
    {  0, 76, 76}, // 136
    {38,  76, 76}, // 137
    {  0, 38, 38}, // 138
    {19,  38, 38}, // 139
    {  0,191,255}, // 140
    {127,223,255}, // 141
    {  0,124,165}, // 142
    {82, 145,165}, // 143
    {  0, 95,127}, // 144
    {63, 111,127}, // 145
    {  0, 57, 76}, // 146
    {38,  66, 76}, // 147
    {  0, 28, 38}, // 148
    {19,  33, 38}, // 149
    {  0,127,255}, // 150
    {127,191,255}, // 151
    {  0, 82,165}, // 152
    {82, 124,165}, // 153
    {  0, 63,127}, // 154
    {63,  95,127}, // 155
    {  0, 38, 76}, // 156
    {38,  57, 76}, // 157
    {  0, 19, 38}, // 158
    {19,  28, 38}, // 159
    {  0, 63,255}, // 160
    {127,159,255}, // 161
    {  0, 41,165}, // 162
    {82, 103,165}, // 163
    {  0, 31,127}, // 164
    {63,  79,127}, // 165
    {  0, 19, 76}, // 166
    {38,  47, 76}, // 167
    {  0,  9, 38}, // 168
    {19,  23, 38}, // 169
    {  0,  0,255}, // 170
    {127,127,255}, // 171
    {  0,  0,165}, // 172
    {82,  82,165}, // 173
    {  0,  0,127}, // 174
    {63,  63,127}, // 175
    {  0,  0, 76}, // 176
    {38,  38, 76}, // 177
    {  0,  0, 38}, // 178
    {19,  19, 38}, // 179
    {63,   0,255}, // 180
    {159,127,255}, // 181
    {41,   0,165}, // 182
    {103, 82,165}, // 183
    {31,   0,127}, // 184
    {79,  63,127}, // 185
    {19,   0, 76}, // 186
    {47,  38, 76}, // 187
    {9,    0, 38}, // 188
    {23,  19, 38}, // 189
    {127,  0,255}, // 190
    {191,127,255}, // 191
    {82,   0,165}, // 192
    {124, 82,165}, // 193
    {63,   0,127}, // 194
    {95,  63,127}, // 195
    {38,   0, 76}, // 196
    {57,  38, 76}, // 197
    {19,   0, 38}, // 198
    {28,  19, 38}, // 199
    {191,  0,255}, // 200
    {223,127,255}, // 201
    {124,  0,165}, // 202
    {145, 82,165}, // 203
    {95,   0,127}, // 204
    {111, 63,127}, // 205
    {57,   0, 76}, // 206
    {66,  38, 76}, // 207
    {28,   0, 38}, // 208
    {33,  19, 38}, // 209
    {255,  0,255}, // 210
    {255,127,255}, // 211
    {165,  0,165}, // 212
    {165, 82,165}, // 213
    {127,  0,127}, // 214
    {127, 63,127}, // 215
    {76,   0, 76}, // 216
    {76,  38, 76}, // 217
    {38,   0, 38}, // 218
    {38,  19, 38}, // 219
    {255,  0,191}, // 220
    {255,127,223}, // 221
    {165,  0,124}, // 222
    {165, 82,145}, // 223
    {127,  0, 95}, // 224
    {127, 63,111}, // 225
    {76,   0, 57}, // 226
    {76,  38, 66}, // 227
    {38,   0, 28}, // 228
    {38,  19, 33}, // 229
    {255,  0,127}, // 230
    {255,127,191}, // 231
    {165,  0, 82}, // 232
    {165, 82,124}, // 233
    {127,  0, 63}, // 234
    {127, 63, 95}, // 235
    {76,   0, 38}, // 236
    {76,  38, 57}, // 237
    {38,   0, 19}, // 238
    {38,  19, 28}, // 239
    {255,  0, 63}, // 240
    {255,127,159}, // 241
    {165,  0, 41}, // 242
    {165, 82,103}, // 243
    {127,  0, 31}, // 244
    {127, 63, 79}, // 245
    {76,   0, 19}, // 246
    {76,  38, 47}, // 247
    {38,   0,  9}, // 248
    {38,  19, 23}, // 249
    { 51, 51, 51}, // 250
    { 80, 80, 80}, // 251
    {105,105,105}, // 252
    {130,130,130}, // 253
    {190,190,190}, // 254
    {255,255,255}, // 255
};

void aciToRgb(int aci, float& r, float& g, float& b) {
    if (aci < 0 || aci > 255) {
        r = 0.8f; g = 0.8f; b = 0.8f; // fallback light gray
        return;
    }
    r = ACI_TABLE[aci][0] / 255.0f;
    g = ACI_TABLE[aci][1] / 255.0f;
    b = ACI_TABLE[aci][2] / 255.0f;
}

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

    // Preview VAO/VBO
    m_previewVao.create();
    m_previewVbo.create();

    m_initialized = true;
    return true;
}

void GlRenderer::getEntityColor(const Entity* entity, float& r, float& g, float& b,
                                 const LayerManager* layers) {
    // 1. Selected entities always cyan
    if (entity->isSelected()) {
        r = EntityColors::SELECT_R;
        g = EntityColors::SELECT_G;
        b = EntityColors::SELECT_B;
        return;
    }

    // 2. Domusong classification overrides DXF color
    switch (entity->classification()) {
        case LineClassification::Oshi:
            r = EntityColors::OSHI_R; g = EntityColors::OSHI_G; b = EntityColors::OSHI_B; return;
        case LineClassification::Knife:
            r = EntityColors::KNIFE_R; g = EntityColors::KNIFE_G; b = EntityColors::KNIFE_B; return;
        case LineClassification::Guide:
            r = EntityColors::GUIDE_R; g = EntityColors::GUIDE_G; b = EntityColors::GUIDE_B; return;
        case LineClassification::Bridge:
            r = EntityColors::BRIDGE_R; g = EntityColors::BRIDGE_G; b = EntityColors::BRIDGE_B; return;
        case LineClassification::Outline:
            r = EntityColors::OUTLINE_R; g = EntityColors::OUTLINE_G; b = EntityColors::OUTLINE_B; return;
        default:
            break;
    }

    // 3. True color (24-bit RGB) takes highest priority for unclassified entities
    if (entity->hasTrueColor()) {
        int tc = entity->trueColor();
        r = ((tc >> 16) & 0xFF) / 255.0f;
        g = ((tc >> 8) & 0xFF) / 255.0f;
        b = (tc & 0xFF) / 255.0f;
        return;
    }

    // 4. ACI color
    int aci = entity->color();

    // BYLAYER (256): use layer color
    if (aci == 256 && layers) {
        const Layer* layer = layers->layer(entity->layerName());
        if (layer) {
            aci = layer->color();
        }
    }

    // BYBLOCK (0): fallback to white
    if (aci == 0) aci = 7;

    // Convert ACI to RGB
    if (aci >= 1 && aci <= 255) {
        aciToRgb(aci, r, g, b);
        return;
    }

    // Fallback
    r = EntityColors::DEFAULT_R;
    g = EntityColors::DEFAULT_G;
    b = EntityColors::DEFAULT_B;
}

void GlRenderer::tessellateCircle(const CircleEntity* circle, std::vector<GlVertex>& verts,
                                    const LayerManager* layers) {
    float r, g, b;
    getEntityColor(circle, r, g, b, layers);
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

void GlRenderer::tessellateArc(const ArcEntity* arc, std::vector<GlVertex>& verts,
                                 const LayerManager* layers) {
    float r, g, b;
    getEntityColor(arc, r, g, b, layers);
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

    const LayerManager* layers = &doc.layers();

    for (EntityId id : doc.entityIds()) {
        const Entity* entity = doc.entity(id);
        if (!entity || !entity->isVisible()) continue;

        float r, g, b;
        getEntityColor(entity, r, g, b, layers);

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
                tessellateCircle(static_cast<const CircleEntity*>(entity), m_entityVerts, layers);
                break;
            case EntityType::Arc:
                tessellateArc(static_cast<const ArcEntity*>(entity), m_entityVerts, layers);
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

void GlRenderer::refreshColors(const Document& doc) {
    if (!m_initialized || m_entityVerts.empty()) return;

    const LayerManager* layers = &doc.layers();
    size_t vi = 0;

    for (EntityId id : doc.entityIds()) {
        const Entity* entity = doc.entity(id);
        if (!entity || !entity->isVisible()) continue;

        float r, g, b;
        getEntityColor(entity, r, g, b, layers);

        // Count how many vertices this entity uses
        size_t vertCount = 0;
        switch (entity->type()) {
            case EntityType::Line: vertCount = 2; break;
            case EntityType::Circle: vertCount = ARC_SEGMENTS * 2; break;
            case EntityType::Arc: {
                auto* arc = static_cast<const ArcEntity*>(entity);
                double startRad = arc->startAngle() * PI / 180.0;
                double endRad = arc->endAngle() * PI / 180.0;
                if (endRad <= startRad) endRad += 2.0 * PI;
                double span = endRad - startRad;
                vertCount = std::max(4, static_cast<int>(ARC_SEGMENTS * span / (2.0 * PI))) * 2;
                break;
            }
            case EntityType::Polyline: {
                auto* poly = static_cast<const PolylineEntity*>(entity);
                size_t segs = poly->vertices().size() > 1 ? poly->vertices().size() - 1 : 0;
                if (poly->isClosed() && poly->vertices().size() > 1) segs++;
                vertCount = segs * 2;
                break;
            }
            case EntityType::Text: vertCount = 4; break;
            default: break;
        }

        // Patch colors in-place
        for (size_t j = 0; j < vertCount && vi + j < m_entityVerts.size(); ++j) {
            m_entityVerts[vi + j].r = r;
            m_entityVerts[vi + j].g = g;
            m_entityVerts[vi + j].b = b;
        }
        vi += vertCount;
    }

    // Upload patched data
    m_entityVao.bind();
    m_entityVbo.bind();
    m_entityVbo.write(0, m_entityVerts.data(),
                      static_cast<int>(m_entityVerts.size() * sizeof(GlVertex)));
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

    // Clamp to pre-allocated max
    if (gridVerts.size() > static_cast<size_t>(GRID_MAX_VERTICES))
        gridVerts.resize(GRID_MAX_VERTICES);
    m_gridVertexCount = static_cast<int>(gridVerts.size());

    m_gridVao.bind();
    m_gridVbo.bind();

    if (!m_gridVboInitialized) {
        // First time: allocate full buffer + set vertex attributes once
        m_gridVbo.allocate(GRID_MAX_VERTICES * static_cast<int>(sizeof(GlVertex)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GlVertex), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex),
                              reinterpret_cast<void*>(2 * sizeof(float)));
        m_gridVboInitialized = true;
    }

    // SubData: update data without reallocating GPU memory
    if (m_gridVertexCount > 0) {
        m_gridVbo.write(0, gridVerts.data(),
                        static_cast<int>(gridVerts.size() * sizeof(GlVertex)));
    }

    m_gridVbo.release();
    m_gridVao.release();
}

void GlRenderer::render(const Camera& camera) {
    if (!m_initialized) return;

    glClearColor(m_bgR, m_bgG, m_bgB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_shader->bind();
    m_shader->setUniformValue(m_projLoc, camera.projectionMatrix());

    // Draw grid first (behind entities) — only rebuild if camera changed
    if (m_gridEnabled) {
        if (isGridCacheDirty(camera)) {
            buildGridVertices(camera);
            updateGridCache(camera);
        }
        if (m_gridVertexCount > 0) {
            m_gridVao.bind();
            glDrawArrays(GL_LINES, 0, m_gridVertexCount);
            m_gridVao.release();
        }
    }

    // Draw entities
    if (m_entityVertexCount > 0) {
        m_entityVao.bind();
        glDrawArrays(GL_LINES, 0, m_entityVertexCount);
        m_entityVao.release();
    }

    // Draw preview (rubber-band lines) on top
    if (m_previewVertexCount > 0) {
        m_previewVao.bind();
        glDrawArrays(GL_LINES, 0, m_previewVertexCount);
        m_previewVao.release();
    }

    m_shader->release();
}

void GlRenderer::updatePreview(const std::vector<std::pair<Point2D, Point2D>>& lines,
                                const Point2D* snapPoint) {
    if (!m_initialized) return;

    // Fast path: nothing to draw
    if (lines.empty() && !snapPoint) {
        m_previewVertexCount = 0;
        return;
    }

    std::vector<GlVertex> verts;
    verts.reserve(lines.size() * 2 + (snapPoint ? 4 : 0));

    // Preview lines in bright green
    for (const auto& [p1, p2] : lines) {
        verts.push_back({static_cast<float>(p1.x), static_cast<float>(p1.y), 0.0f, 1.0f, 0.5f});
        verts.push_back({static_cast<float>(p2.x), static_cast<float>(p2.y), 0.0f, 1.0f, 0.5f});
    }

    // Snap indicator: small cross at snap point
    if (snapPoint) {
        float sx = static_cast<float>(snapPoint->x);
        float sy = static_cast<float>(snapPoint->y);
        float sz = 2.0f;
        verts.push_back({sx - sz, sy, 1.0f, 1.0f, 0.0f});
        verts.push_back({sx + sz, sy, 1.0f, 1.0f, 0.0f});
        verts.push_back({sx, sy - sz, 1.0f, 1.0f, 0.0f});
        verts.push_back({sx, sy + sz, 1.0f, 1.0f, 0.0f});
    }

    m_previewVertexCount = static_cast<int>(verts.size());

    m_previewVao.bind();
    m_previewVbo.bind();
    m_previewVbo.allocate(verts.data(), static_cast<int>(verts.size() * sizeof(GlVertex)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GlVertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    m_previewVbo.release();
    m_previewVao.release();
}

bool GlRenderer::isGridCacheDirty(const Camera& camera) const {
    if (m_gridDirty) return true;
    constexpr double EPS = 1e-6;
    return std::abs(camera.zoom() - m_lastGridZoom) > EPS
        || std::abs(camera.center().x - m_lastGridCenter.x) > EPS
        || std::abs(camera.center().y - m_lastGridCenter.y) > EPS
        || camera.viewportWidth() != m_lastGridVpW
        || camera.viewportHeight() != m_lastGridVpH;
}

void GlRenderer::updateGridCache(const Camera& camera) {
    m_lastGridCenter = camera.center();
    m_lastGridZoom = camera.zoom();
    m_lastGridVpW = camera.viewportWidth();
    m_lastGridVpH = camera.viewportHeight();
    m_gridDirty = false;
}

void GlRenderer::cleanup() {
    if (!m_initialized) return;
    m_entityVbo.destroy();
    m_entityVao.destroy();
    m_gridVbo.destroy();
    m_gridVao.destroy();
    m_previewVbo.destroy();
    m_previewVao.destroy();
    m_shader.reset();
    m_initialized = false;
}

} // namespace yjcad
