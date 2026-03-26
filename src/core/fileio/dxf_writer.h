#pragma once

#include "core/document/document.h"
#include <string>

namespace yjcad {

struct DxfSaveResult {
    bool success = false;
    std::string errorMessage;
    size_t entityCount = 0;
};

/// DXF Writer — saves a Document to DXF file using libdxfrw
/// Uses atomic save pattern: write to temp file, then rename.
class DxfWriter {
public:
    explicit DxfWriter(const Document& doc);

    /// Save document to DXF file (atomic: temp → rename)
    DxfSaveResult save(const std::string& filePath);

    /// Generate filename following 도무송 convention:
    /// MMDD-순번 업체명 도면명 절수.DXF
    static std::string generateFilename(int month, int day, int seq,
                                         const std::string& company,
                                         const std::string& drawingName,
                                         const std::string& jeolsu);

private:
    const Document& m_doc;
};

} // namespace yjcad
