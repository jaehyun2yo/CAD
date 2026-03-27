#pragma once

#include "core/document/document.h"
#include "core/document/entity.h"
#include <string>
#include <sstream>

namespace yjcad {

/// NcGenerator — converts Document entities to NC G-code text.
///
/// Supported entity types: LineEntity, ArcEntity, PolylineEntity.
/// CircleEntity and TextEntity are silently skipped.
///
/// Coordinate system: X+ right, Y+ up, absolute mode (G90), mm (G71).
///
/// Arc convention:
///   G02 = clockwise (CW)
///   G03 = counter-clockwise (CCW)
///   I, J = arc center offset from current position (incremental).
class NcGenerator {
public:
    /// NC generation configuration.
    struct NcConfig {
        std::string startTag = "%";   // program start delimiter
        std::string endTag   = "%";   // program end delimiter
        std::string header   = "";    // optional M-code header (e.g. "M03 S1000")
        std::string footer   = "M30"; // program end command
        double feedRate      = 100.0; // cutting feed rate (mm/min)
        int    programNumber = 1;     // O-number (0 = omit)
    };

    NcGenerator() = default;

    /// Generate G-code string from all visible entities in the document.
    /// Entities are processed in the order returned by Document::entityIds().
    std::string generate(const Document& doc, const NcConfig& config = NcConfig{});

private:
    void writePreamble(std::ostringstream& out, const NcConfig& cfg);
    void writePostamble(std::ostringstream& out, const NcConfig& cfg);

    void writeEntity(const Entity* e, std::ostringstream& out, const NcConfig& cfg);
    void writeLine(const LineEntity* l, std::ostringstream& out, const NcConfig& cfg);
    void writeArc(const ArcEntity* a, std::ostringstream& out, const NcConfig& cfg);
    void writePolyline(const PolylineEntity* p, std::ostringstream& out, const NcConfig& cfg);

    /// Format a coordinate value to 3 decimal places.
    static std::string fmt(double v);

    double m_curX = 0.0;
    double m_curY = 0.0;
};

} // namespace yjcad
