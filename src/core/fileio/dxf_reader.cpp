#include "dxf_reader.h"
#include <filesystem>

namespace yjcad {

DxfReader::DxfReader(Document& doc) : m_doc(doc) {}

DxfLoadResult DxfReader::load(const std::string& filePath) {
    DxfLoadResult result;
    m_entityCount = 0;
    m_warnings.clear();

    // Validate file exists
    if (!std::filesystem::exists(filePath)) {
        result.errorMessage = "File not found: " + filePath;
        return result;
    }

    // Clear document before loading
    m_doc.clear();

    // Parse DXF using libdxfrw
    dxfRW reader(filePath.c_str());
    bool ok = reader.read(this, false); // false = ASCII mode

    if (!ok) {
        result.errorMessage = "Failed to parse DXF file: " + filePath;
        return result;
    }

    result.success = true;
    result.entityCount = m_entityCount;
    result.layerCount = m_doc.layers().count();
    result.warnings = m_warnings;

    m_doc.setFilePath(filePath);
    m_doc.setModified(false);

    return result;
}

void DxfReader::applyCommonProperties(Entity* entity, const DRW_Entity& drwEntity) {
    entity->setLayerName(drwEntity.layer);
    entity->setColor(drwEntity.color);
    entity->setLineWidth(drwEntity.lWeight / 100.0); // DXF stores in 1/100mm

    // Ensure layer exists
    if (!m_doc.layers().layer(drwEntity.layer)) {
        m_doc.layers().addLayer(drwEntity.layer, drwEntity.color);
    }
}

// ── Callbacks ──

void DxfReader::addHeader(const DRW_Header* /*data*/) {
    // Could extract units, extents, etc. — skip for now
}

void DxfReader::addLayer(const DRW_Layer& data) {
    m_doc.layers().addLayer(data.name, data.color);
    auto* layer = m_doc.layers().layer(data.name);
    if (layer) {
        layer->setVisible(!data.plotF); // plotF=off means not visible
        layer->setFrozen(data.flags & 0x01);
        layer->setLocked(data.flags & 0x04);
    }
}

void DxfReader::addLine(const DRW_Line& data) {
    auto id = m_doc.nextId();
    Point2D start(data.basePoint.x, data.basePoint.y);
    Point2D end(data.secPoint.x, data.secPoint.y);
    auto entity = std::make_unique<LineEntity>(id, start, end);
    applyCommonProperties(entity.get(), data);
    m_doc.addEntity(std::move(entity));
    ++m_entityCount;
}

void DxfReader::addCircle(const DRW_Circle& data) {
    auto id = m_doc.nextId();
    Point2D center(data.basePoint.x, data.basePoint.y);
    auto entity = std::make_unique<CircleEntity>(id, center, data.radious);
    applyCommonProperties(entity.get(), data);
    m_doc.addEntity(std::move(entity));
    ++m_entityCount;
}

void DxfReader::addArc(const DRW_Arc& data) {
    auto id = m_doc.nextId();
    Point2D center(data.basePoint.x, data.basePoint.y);
    // libdxfrw stores angles in radians, convert to degrees
    double startDeg = data.staangle * 180.0 / 3.14159265358979323846;
    double endDeg = data.endangle * 180.0 / 3.14159265358979323846;
    auto entity = std::make_unique<ArcEntity>(id, center, data.radious, startDeg, endDeg);
    applyCommonProperties(entity.get(), data);
    m_doc.addEntity(std::move(entity));
    ++m_entityCount;
}

void DxfReader::addLWPolyline(const DRW_LWPolyline& data) {
    auto id = m_doc.nextId();
    std::vector<Point2D> vertices;
    vertices.reserve(data.vertlist.size());
    for (const auto& vertex : data.vertlist) {
        vertices.emplace_back(vertex->x, vertex->y);
    }
    bool closed = (data.flags & 0x01) != 0;
    auto entity = std::make_unique<PolylineEntity>(id, vertices, closed);
    applyCommonProperties(entity.get(), data);
    m_doc.addEntity(std::move(entity));
    ++m_entityCount;
}

void DxfReader::addPolyline(const DRW_Polyline& data) {
    // Heavy polyline (2D/3D) — convert to simple polyline
    auto id = m_doc.nextId();
    std::vector<Point2D> vertices;
    vertices.reserve(data.vertlist.size());
    for (const auto& vertex : data.vertlist) {
        vertices.emplace_back(vertex->basePoint.x, vertex->basePoint.y);
    }
    bool closed = (data.flags & 0x01) != 0;
    auto entity = std::make_unique<PolylineEntity>(id, vertices, closed);
    applyCommonProperties(entity.get(), data);
    m_doc.addEntity(std::move(entity));
    ++m_entityCount;
}

void DxfReader::addText(const DRW_Text& data) {
    auto id = m_doc.nextId();
    Point2D pos(data.basePoint.x, data.basePoint.y);
    auto entity = std::make_unique<TextEntity>(id, pos, data.text, data.height);
    entity->setRotation(data.angle);
    applyCommonProperties(entity.get(), data);
    m_doc.addEntity(std::move(entity));
    ++m_entityCount;
}

void DxfReader::addMText(const DRW_MText& data) {
    auto id = m_doc.nextId();
    Point2D pos(data.basePoint.x, data.basePoint.y);
    auto entity = std::make_unique<TextEntity>(id, pos, data.text, data.height);
    entity->setRotation(data.angle);
    applyCommonProperties(entity.get(), data);
    m_doc.addEntity(std::move(entity));
    ++m_entityCount;
}

void DxfReader::addPoint(const DRW_Point& data) {
    // Store as a zero-length line (simplification)
    auto id = m_doc.nextId();
    Point2D pos(data.basePoint.x, data.basePoint.y);
    auto entity = std::make_unique<LineEntity>(id, pos, pos);
    applyCommonProperties(entity.get(), data);
    m_doc.addEntity(std::move(entity));
    ++m_entityCount;
}

void DxfReader::addInsert(const DRW_Insert& data) {
    // Block references — store position as warning, skip for M1
    m_warnings.push_back("INSERT (block ref) skipped: " + data.name +
                         " at (" + std::to_string(data.basePoint.x) + ", " +
                         std::to_string(data.basePoint.y) + ")");
}

} // namespace yjcad
