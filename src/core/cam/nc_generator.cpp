#include "nc_generator.h"
#include <cmath>
#include <iomanip>
#include <sstream>

static constexpr double PI = 3.14159265358979323846;

namespace yjcad {

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────

std::string NcGenerator::fmt(double v)
{
    std::ostringstream s;
    s << std::fixed << std::setprecision(3) << v;
    return s.str();
}

// ─────────────────────────────────────────────
// Public entry point
// ─────────────────────────────────────────────

std::string NcGenerator::generate(const Document& doc, const NcConfig& config)
{
    m_curX = 0.0;
    m_curY = 0.0;

    std::ostringstream out;
    writePreamble(out, config);

    for (EntityId id : doc.entityIds()) {
        const Entity* e = doc.entity(id);
        if (!e || !e->isVisible()) continue;
        writeEntity(e, out, config);
    }

    writePostamble(out, config);
    return out.str();
}

// ─────────────────────────────────────────────
// Preamble / Postamble
// ─────────────────────────────────────────────

void NcGenerator::writePreamble(std::ostringstream& out, const NcConfig& cfg)
{
    out << cfg.startTag << "\n";

    if (cfg.programNumber > 0) {
        out << "O" << std::setw(4) << std::setfill('0') << cfg.programNumber << "\n";
    }

    out << "G90 G71\n"; // absolute coords, mm
    out << "G17\n";     // XY plane

    if (!cfg.header.empty()) {
        out << cfg.header << "\n";
    }

    // Rapid to origin
    out << "G00 X0.000 Y0.000\n";
}

void NcGenerator::writePostamble(std::ostringstream& out, const NcConfig& cfg)
{
    // Rapid home
    out << "G00 X0.000 Y0.000\n";

    if (!cfg.footer.empty()) {
        out << cfg.footer << "\n";
    }

    out << cfg.endTag << "\n";
}

// ─────────────────────────────────────────────
// Entity dispatch
// ─────────────────────────────────────────────

void NcGenerator::writeEntity(const Entity* e, std::ostringstream& out, const NcConfig& cfg)
{
    switch (e->type()) {
    case EntityType::Line:
        writeLine(static_cast<const LineEntity*>(e), out, cfg);
        break;
    case EntityType::Arc:
        writeArc(static_cast<const ArcEntity*>(e), out, cfg);
        break;
    case EntityType::Polyline:
        writePolyline(static_cast<const PolylineEntity*>(e), out, cfg);
        break;
    default:
        // Circle, Text, Insert, Point — not supported for NC output
        break;
    }
}

// ─────────────────────────────────────────────
// Line
// ─────────────────────────────────────────────

void NcGenerator::writeLine(const LineEntity* l, std::ostringstream& out, const NcConfig& cfg)
{
    const Point2D& s = l->start();
    const Point2D& e = l->end();

    // Rapid to start if not already there
    if (std::abs(s.x - m_curX) > 1e-6 || std::abs(s.y - m_curY) > 1e-6) {
        out << "G00 X" << fmt(s.x) << " Y" << fmt(s.y) << "\n";
        m_curX = s.x;
        m_curY = s.y;
    }

    // Linear interpolation to end
    out << "G01 X" << fmt(e.x) << " Y" << fmt(e.y)
        << " F" << fmt(cfg.feedRate) << "\n";

    m_curX = e.x;
    m_curY = e.y;
}

// ─────────────────────────────────────────────
// Arc
// ─────────────────────────────────────────────

void NcGenerator::writeArc(const ArcEntity* a, std::ostringstream& out, const NcConfig& cfg)
{
    const Point2D& c = a->center();
    const double   r = a->radius();
    const double   startRad = a->startAngle() * PI / 180.0;
    const double   endRad   = a->endAngle()   * PI / 180.0;

    Point2D startPt{c.x + r * std::cos(startRad), c.y + r * std::sin(startRad)};
    Point2D endPt  {c.x + r * std::cos(endRad),   c.y + r * std::sin(endRad)};

    // I, J are offsets from current position (start point) to arc center
    double I = c.x - startPt.x;
    double J = c.y - startPt.y;

    // Rapid to arc start
    if (std::abs(startPt.x - m_curX) > 1e-6 || std::abs(startPt.y - m_curY) > 1e-6) {
        out << "G00 X" << fmt(startPt.x) << " Y" << fmt(startPt.y) << "\n";
    }

    // Determine CW (G02) vs CCW (G03):
    // DXF arcs are always CCW by convention.
    out << "G03 X" << fmt(endPt.x) << " Y" << fmt(endPt.y)
        << " I" << fmt(I) << " J" << fmt(J)
        << " F" << fmt(cfg.feedRate) << "\n";

    m_curX = endPt.x;
    m_curY = endPt.y;
}

// ─────────────────────────────────────────────
// Polyline
// ─────────────────────────────────────────────

void NcGenerator::writePolyline(const PolylineEntity* p, std::ostringstream& out, const NcConfig& cfg)
{
    const auto& verts = p->vertices();
    if (verts.empty()) return;

    // Rapid to first vertex
    const Point2D& first = verts[0];
    if (std::abs(first.x - m_curX) > 1e-6 || std::abs(first.y - m_curY) > 1e-6) {
        out << "G00 X" << fmt(first.x) << " Y" << fmt(first.y) << "\n";
        m_curX = first.x;
        m_curY = first.y;
    }

    // Linear moves for each subsequent vertex
    for (size_t i = 1; i < verts.size(); ++i) {
        out << "G01 X" << fmt(verts[i].x) << " Y" << fmt(verts[i].y)
            << " F" << fmt(cfg.feedRate) << "\n";
        m_curX = verts[i].x;
        m_curY = verts[i].y;
    }

    // Close if needed
    if (p->isClosed() && verts.size() > 1) {
        out << "G01 X" << fmt(first.x) << " Y" << fmt(first.y)
            << " F" << fmt(cfg.feedRate) << "\n";
        m_curX = first.x;
        m_curY = first.y;
    }
}

} // namespace yjcad
