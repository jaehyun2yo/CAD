#pragma once

#include "core/document/document.h"
#include <libdxfrw/drw_interface.h>
#include <libdxfrw/libdxfrw.h>
#include <string>
#include <vector>

namespace yjcad {

/// Result of a DXF file load operation
struct DxfLoadResult {
    bool success = false;
    std::string errorMessage;
    size_t entityCount = 0;
    size_t layerCount = 0;
    std::vector<std::string> warnings;
};

/// DXF Reader — loads DXF files into a Document using libdxfrw
class DxfReader : private DRW_Interface {
public:
    explicit DxfReader(Document& doc);

    /// Load a DXF file into the document
    DxfLoadResult load(const std::string& filePath);

private:
    // ── DRW_Interface callbacks ──
    void addHeader(const DRW_Header* data) override;
    void addLayer(const DRW_Layer& data) override;
    void addLine(const DRW_Line& data) override;
    void addCircle(const DRW_Circle& data) override;
    void addArc(const DRW_Arc& data) override;
    void addLWPolyline(const DRW_LWPolyline& data) override;
    void addPolyline(const DRW_Polyline& data) override;
    void addText(const DRW_Text& data) override;
    void addMText(const DRW_MText& data) override;
    void addPoint(const DRW_Point& data) override;
    void addInsert(const DRW_Insert& data) override;

    // ── Stubs for unused callbacks (required by DRW_Interface) ──
    void addLType(const DRW_LType&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}
    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeAppId() override {}
    void writeObjects() override {}

    // ── Helper ──
    void applyCommonProperties(Entity* entity, const DRW_Entity& drwEntity);

    Document& m_doc;
    size_t m_entityCount = 0;
    std::vector<std::string> m_warnings;
};

} // namespace yjcad
