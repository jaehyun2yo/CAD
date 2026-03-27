#include "box_templates.h"
#include <cmath>

namespace yjcad {

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────

BoxTemplates::BoxTemplates(Document& doc)
    : m_doc(doc)
{}

// ─────────────────────────────────────────────
// Low-level helpers
// ─────────────────────────────────────────────

void BoxTemplates::addKnifeLine(const Point2D& p1, const Point2D& p2)
{
    auto e = std::make_unique<LineEntity>(m_doc.nextId(), p1, p2);
    e->setClassification(LineClassification::Knife);
    e->setLayerName("칼선");
    e->setColor(7); // white (AutoCAD convention)
    m_doc.addEntity(std::move(e));
}

void BoxTemplates::addOshiLine(const Point2D& p1, const Point2D& p2)
{
    auto e = std::make_unique<LineEntity>(m_doc.nextId(), p1, p2);
    e->setClassification(LineClassification::Oshi);
    e->setLayerName("오시");
    e->setColor(1); // red (AutoCAD color index 1)
    m_doc.addEntity(std::move(e));
}

void BoxTemplates::addKnifeRect(double x, double y, double w, double h)
{
    // bottom, top, left, right
    addKnifeLine({x,     y    }, {x + w, y    });
    addKnifeLine({x,     y + h}, {x + w, y + h});
    addKnifeLine({x,     y    }, {x,     y + h});
    addKnifeLine({x + w, y    }, {x + w, y + h});
}

void BoxTemplates::addOshiRect(double x, double y, double w, double h)
{
    addOshiLine({x,     y    }, {x + w, y    });
    addOshiLine({x,     y + h}, {x + w, y + h});
    addOshiLine({x,     y    }, {x,     y + h});
    addOshiLine({x + w, y    }, {x + w, y + h});
}

// ─────────────────────────────────────────────
// A형 (맞뚜껑) 박스
// ─────────────────────────────────────────────
//
// Cross layout (origin = floor center):
//
//        ┌──────┐
//        │  lid  │
//   ┌────┼──────┼────┬──────┐
//   │left│floor │ rt │ tab  │
//   └────┼──────┼────┴──────┘
//        │bottom│
//        └──────┘
//
// W = width, H = height(seHeight), D = depth
//
void BoxTemplates::createABox(const BoxParams& p)
{
    const double W  = p.width;
    const double H  = p.height;
    const double D  = p.depth;
    const double T  = p.thickness;
    const double tab = D * 0.4; // glue tab width

    // Floor panel: centered at origin
    const double fx = -W / 2.0;
    const double fy = -H / 2.0;

    // ── Oshi lines: fold borders ──

    // Floor ↔ bottom side panel
    addOshiLine({fx, fy}, {fx + W, fy});
    // Floor ↔ top side panel
    addOshiLine({fx, fy + H}, {fx + W, fy + H});
    // Floor ↔ left side panel
    addOshiLine({fx, fy}, {fx, fy + H});
    // Floor ↔ right side panel (and tab separator)
    addOshiLine({fx + W, fy}, {fx + W, fy + H});

    // Bottom side panel ↔ bottom lip
    addOshiLine({fx, fy - D}, {fx + W, fy - D});
    // Top side panel ↔ lid
    addOshiLine({fx, fy + H + D}, {fx + W, fy + H + D});
    // Left side ↔ left bottom/top flaps
    addOshiLine({fx - D, fy}, {fx - D, fy + H});
    // Right tab inner fold
    addOshiLine({fx + W + tab, fy}, {fx + W + tab, fy + H});

    // ── Knife lines: outer contour ──

    // Bottom side bottom edge
    addKnifeLine({fx, fy - D}, {fx + W, fy - D});
    // Top side + lid top edge
    addKnifeLine({fx, fy + H + D + D}, {fx + W, fy + H + D + D});

    // Left outer edge (from bottom flap bottom to top flap top)
    addKnifeLine({fx - D, fy - D}, {fx - D, fy + H + D});
    // Right outer edge (tab)
    addKnifeLine({fx + W + tab, fy - D}, {fx + W + tab, fy + H + D});

    // Bottom-left corner notch (left flap of bottom panel)
    addKnifeLine({fx - D, fy - D}, {fx, fy - D});
    // Bottom-right corner join
    addKnifeLine({fx + W, fy - D}, {fx + W + tab, fy - D});

    // Top-left corner notch
    addKnifeLine({fx - D, fy + H + D}, {fx, fy + H + D});
    // Top-right corner join
    addKnifeLine({fx + W, fy + H + D}, {fx + W + tab, fy + H + D});

    // Left vertical edges
    addKnifeLine({fx - D, fy - D}, {fx - D, fy});
    addKnifeLine({fx - D, fy + H}, {fx - D, fy + H + D});

    // Lid top edge (2 * D above top fold line)
    addKnifeLine({fx, fy + H + D + D}, {fx + W, fy + H + D + D});

    // Left/right edges of bottom and lid panels
    addKnifeLine({fx, fy - D}, {fx, fy - D - T});
    addKnifeLine({fx + W, fy - D}, {fx + W, fy - D - T});
    addKnifeLine({fx, fy + H + D + D - T}, {fx, fy + H + D + D});
    addKnifeLine({fx + W, fy + H + D + D - T}, {fx + W, fy + H + D + D});
}

// ─────────────────────────────────────────────
// 접어넣기 (Tuck-end) 박스
// ─────────────────────────────────────────────
void BoxTemplates::createTuckBox(const BoxParams& p)
{
    const double W   = p.width;
    const double H   = p.height;
    const double D   = p.depth;
    const double T   = p.thickness;
    const double tuck = D * 0.9; // tuck flap depth (slightly shorter than panel)
    const double lock = D * 0.3; // lock tab height

    const double fx = -W / 2.0;
    const double fy = -H / 2.0;

    // ── Oshi: main panel fold lines ──
    addOshiLine({fx,     fy}, {fx + W, fy});
    addOshiLine({fx,     fy + H}, {fx + W, fy + H});
    addOshiLine({fx,     fy}, {fx,     fy + H});
    addOshiLine({fx + W, fy}, {fx + W, fy + H});

    // Side panel outer folds
    addOshiLine({fx, fy - D}, {fx + W, fy - D});
    addOshiLine({fx, fy + H + D}, {fx + W, fy + H + D});

    // Tuck flap fold lines
    addOshiLine({fx, fy - D - tuck}, {fx + W, fy - D - tuck});
    addOshiLine({fx, fy + H + D + tuck}, {fx + W, fy + H + D + tuck});

    // Left/right dust flap folds
    addOshiLine({fx - D, fy - D}, {fx - D, fy + H + D});

    // Glue tab
    addOshiLine({fx + W + D * 0.35, fy}, {fx + W + D * 0.35, fy + H});

    // ── Knife: outer contour ──

    // Tuck flap outer edges
    double btm = fy - D - tuck;
    double top = fy + H + D + tuck;

    addKnifeRect(fx, btm, W, top - btm);

    // Tuck flap side cutouts (notch to allow folding)
    addKnifeLine({fx, fy - D}, {fx - T, fy - D});
    addKnifeLine({fx + W, fy - D}, {fx + W + T, fy - D});
    addKnifeLine({fx, fy + H + D}, {fx - T, fy + H + D});
    addKnifeLine({fx + W, fy + H + D}, {fx + W + T, fy + H + D});

    // Lock tab notch at bottom tuck
    addKnifeLine({fx + W * 0.3, btm}, {fx + W * 0.3, btm - lock});
    addKnifeLine({fx + W * 0.7, btm}, {fx + W * 0.7, btm - lock});
    addKnifeLine({fx + W * 0.3, btm - lock}, {fx + W * 0.7, btm - lock});

    // Lock tab notch at top tuck
    addKnifeLine({fx + W * 0.3, top}, {fx + W * 0.3, top + lock});
    addKnifeLine({fx + W * 0.7, top}, {fx + W * 0.7, top + lock});
    addKnifeLine({fx + W * 0.3, top + lock}, {fx + W * 0.7, top + lock});

    // Dust flaps (small side panels)
    double dfw = D * 0.9;
    double dfh = D * 0.85;
    // bottom dust flaps
    addKnifeLine({fx - dfw, fy - D}, {fx,     fy - D});
    addKnifeLine({fx - dfw, fy    }, {fx - dfw, fy - D});
    addKnifeLine({fx - dfw, fy    }, {fx,      fy    });
    addKnifeLine({fx + W, fy - D}, {fx + W + dfw, fy - D});
    addKnifeLine({fx + W + dfw, fy - D}, {fx + W + dfw, fy});
    // top dust flaps
    addKnifeLine({fx - dfw, fy + H + D}, {fx,          fy + H + D});
    addKnifeLine({fx - dfw, fy + H},     {fx - dfw,    fy + H + D});
    addKnifeLine({fx - dfw, fy + H},     {fx,          fy + H});
    addKnifeLine({fx + W,   fy + H + D}, {fx + W + dfw, fy + H + D});
    addKnifeLine({fx + W + dfw, fy + H}, {fx + W + dfw, fy + H + D});

    // Glue tab outer edge
    addKnifeLine({fx + W + D * 0.35, fy}, {fx + W + D * 0.35, fy + H});
    addKnifeLine({fx + W,  fy}, {fx + W + D * 0.35, fy});
    addKnifeLine({fx + W,  fy + H}, {fx + W + D * 0.35, fy + H});
}

// ─────────────────────────────────────────────
// 트레이
// ─────────────────────────────────────────────
void BoxTemplates::createTray(const BoxParams& p)
{
    const double W = p.width;
    const double H = p.height;
    const double D = p.depth;

    const double fx = -W / 2.0;
    const double fy = -H / 2.0;

    // ── Oshi: fold lines (floor boundary) ──
    addOshiLine({fx, fy}, {fx + W, fy});
    addOshiLine({fx, fy + H}, {fx + W, fy + H});
    addOshiLine({fx, fy}, {fx, fy + H});
    addOshiLine({fx + W, fy}, {fx + W, fy + H});

    // ── Knife: outer perimeter ──
    // Side panels extend by D in each direction
    // Corner squares are cut away, leaving triangular "귀접기" notches

    // Bottom side top/bottom
    addKnifeLine({fx, fy - D}, {fx + W, fy - D});
    // Top side top/bottom
    addKnifeLine({fx, fy + H + D}, {fx + W, fy + H + D});
    // Left side outer
    addKnifeLine({fx - D, fy}, {fx - D, fy + H});
    // Right side outer
    addKnifeLine({fx + W + D, fy}, {fx + W + D, fy + H});

    // Corner notches (diagonal cuts for 귀접기)
    // Bottom-left
    addKnifeLine({fx,     fy - D}, {fx - D, fy});
    // Bottom-right
    addKnifeLine({fx + W, fy - D}, {fx + W + D, fy});
    // Top-left
    addKnifeLine({fx,     fy + H + D}, {fx - D, fy + H});
    // Top-right
    addKnifeLine({fx + W, fy + H + D}, {fx + W + D, fy + H});

    // Vertical edges of bottom side panel
    addKnifeLine({fx,     fy}, {fx,     fy - D});
    addKnifeLine({fx + W, fy}, {fx + W, fy - D});
    // Vertical edges of top side panel
    addKnifeLine({fx,     fy + H}, {fx,     fy + H + D});
    addKnifeLine({fx + W, fy + H}, {fx + W, fy + H + D});
}

// ─────────────────────────────────────────────
// 자동잠금 박스
// ─────────────────────────────────────────────
void BoxTemplates::createAutoLock(const BoxParams& p)
{
    const double W   = p.width;
    const double H   = p.height;
    const double D   = p.depth;
    const double T   = p.thickness;
    const double tab = D * 0.4;
    const double lock = W * 0.25; // auto-lock tab size

    const double fx = -W / 2.0;
    const double fy = -H / 2.0;

    // ── Oshi: main fold lines ──
    addOshiLine({fx,     fy},     {fx + W, fy});
    addOshiLine({fx,     fy + H}, {fx + W, fy + H});
    addOshiLine({fx,     fy},     {fx,     fy + H});
    addOshiLine({fx + W, fy},     {fx + W, fy + H});

    // Bottom side panel
    addOshiLine({fx, fy - D}, {fx + W, fy - D});
    // Top side + lid fold
    addOshiLine({fx, fy + H + D}, {fx + W, fy + H + D});

    // Auto-lock inner folds (bottom)
    double lockDepth = D * 0.5;
    addOshiLine({fx + lock, fy - D + lockDepth}, {fx + W - lock, fy - D + lockDepth});
    addOshiLine({fx + lock, fy - lockDepth},     {fx + lock,     fy - D + lockDepth});
    addOshiLine({fx + W - lock, fy - lockDepth}, {fx + W - lock, fy - D + lockDepth});

    // Left/right dust flap folds
    addOshiLine({fx - D, fy - D}, {fx - D, fy + H + D});
    addOshiLine({fx + W + tab, fy}, {fx + W + tab, fy + H});

    // ── Knife: outer contour ──
    addKnifeLine({fx, fy - D}, {fx + W, fy - D});
    addKnifeLine({fx, fy + H + D + D}, {fx + W, fy + H + D + D});

    // Left outer
    addKnifeLine({fx - D, fy - D}, {fx - D, fy + H + D});
    // Right tab
    addKnifeLine({fx + W + tab, fy - D}, {fx + W + tab, fy + H + D});

    // Corner connections
    addKnifeLine({fx - D, fy - D}, {fx,     fy - D});
    addKnifeLine({fx + W, fy - D}, {fx + W + tab, fy - D});
    addKnifeLine({fx - D, fy + H + D}, {fx, fy + H + D});
    addKnifeLine({fx + W, fy + H + D}, {fx + W + tab, fy + H + D});

    // Auto-lock tab cutouts on the bottom flap
    addKnifeLine({fx + lock, fy - D},            {fx + lock, fy - D + lockDepth});
    addKnifeLine({fx + W - lock, fy - D},        {fx + W - lock, fy - D + lockDepth});
    addKnifeLine({fx + lock, fy - D + lockDepth},{fx + W - lock, fy - D + lockDepth});

    // Auto-lock receiver slots on the floor panel
    double slotW = lock * 0.6;
    double slotH = lockDepth * 0.8;
    // left slot
    addKnifeRect(fx + lock * 0.2, fy - slotH * 0.5, slotW, slotH);
    // right slot
    addKnifeRect(fx + W - lock * 0.2 - slotW, fy - slotH * 0.5, slotW, slotH);

    // Lid (same height as bottom panel but no lock)
    addKnifeLine({fx, fy + H + D + D}, {fx + W, fy + H + D + D});
    addKnifeLine({fx, fy + H + D},     {fx,     fy + H + D + D});
    addKnifeLine({fx + W, fy + H + D}, {fx + W, fy + H + D + D});
}

// ─────────────────────────────────────────────
// 선물 박스 (상하 분리형 뚜껑)
// ─────────────────────────────────────────────
void BoxTemplates::createGiftBox(const BoxParams& p)
{
    const double W    = p.width;
    const double H    = p.height;
    const double D    = p.depth;
    const double T    = p.thickness;
    const double lidH = D * 0.4;   // lid body height
    const double gap  = 5.0;       // gap between base and lid patterns

    // ── Base (tray-style bottom) ──
    {
        const double fx = -W / 2.0;
        const double fy = -H / 2.0;

        // Fold lines
        addOshiLine({fx,     fy},     {fx + W, fy});
        addOshiLine({fx,     fy + H}, {fx + W, fy + H});
        addOshiLine({fx,     fy},     {fx,     fy + H});
        addOshiLine({fx + W, fy},     {fx + W, fy + H});

        // Outer cut (diagonal corners for 귀접기)
        addKnifeLine({fx, fy - D},     {fx + W, fy - D});
        addKnifeLine({fx, fy + H + D}, {fx + W, fy + H + D});
        addKnifeLine({fx - D, fy},     {fx - D, fy + H});
        addKnifeLine({fx + W + D, fy}, {fx + W + D, fy + H});

        addKnifeLine({fx,     fy - D},     {fx - D, fy});
        addKnifeLine({fx + W, fy - D},     {fx + W + D, fy});
        addKnifeLine({fx,     fy + H + D}, {fx - D, fy + H});
        addKnifeLine({fx + W, fy + H + D}, {fx + W + D, fy + H});

        // Vertical fills
        addKnifeLine({fx,     fy}, {fx,     fy - D});
        addKnifeLine({fx + W, fy}, {fx + W, fy - D});
        addKnifeLine({fx,     fy + H}, {fx,     fy + H + D});
        addKnifeLine({fx + W, fy + H}, {fx + W, fy + H + D});
    }

    // ── Lid (slightly larger, shallow tray) ──
    // Place lid pattern above base with a gap
    {
        const double lwOff = T;      // lid is T larger on each side → overlap fit
        const double lW = W + 2.0 * lwOff;
        const double lH = H + 2.0 * lwOff;
        const double offsetY = H / 2.0 + D + gap + lH / 2.0 + lidH;

        const double fx = -lW / 2.0;
        const double fy = offsetY - lH / 2.0;

        // Fold lines
        addOshiLine({fx,      fy},      {fx + lW, fy});
        addOshiLine({fx,      fy + lH}, {fx + lW, fy + lH});
        addOshiLine({fx,      fy},      {fx,      fy + lH});
        addOshiLine({fx + lW, fy},      {fx + lW, fy + lH});

        // Lid outer cut (diagonal corners)
        addKnifeLine({fx,      fy - lidH},      {fx + lW, fy - lidH});
        addKnifeLine({fx,      fy + lH + lidH}, {fx + lW, fy + lH + lidH});
        addKnifeLine({fx - lidH, fy},      {fx - lidH, fy + lH});
        addKnifeLine({fx + lW + lidH, fy}, {fx + lW + lidH, fy + lH});

        addKnifeLine({fx,      fy - lidH},      {fx - lidH, fy});
        addKnifeLine({fx + lW, fy - lidH},      {fx + lW + lidH, fy});
        addKnifeLine({fx,      fy + lH + lidH}, {fx - lidH, fy + lH});
        addKnifeLine({fx + lW, fy + lH + lidH}, {fx + lW + lidH, fy + lH});

        addKnifeLine({fx,      fy}, {fx,      fy - lidH});
        addKnifeLine({fx + lW, fy}, {fx + lW, fy - lidH});
        addKnifeLine({fx,      fy + lH}, {fx,      fy + lH + lidH});
        addKnifeLine({fx + lW, fy + lH}, {fx + lW, fy + lH + lidH});
    }
}

} // namespace yjcad
