#pragma once

#include "core/document/document.h"
#include "core/document/entity.h"

namespace yjcad {

/// BoxTemplates — generates flat-pattern (전개도) entities for common domusong box types.
/// All entities are placed with the floor center at origin (0, 0).
/// Knife lines (칼선) use LineClassification::Knife.
/// Oshi lines (오시/접선) use LineClassification::Oshi.
class BoxTemplates {
public:
    explicit BoxTemplates(Document& doc);

    /// Parameters common to all box templates.
    struct BoxParams {
        double width;           // 가로 (mm)
        double height;          // 세로 (mm)
        double depth;           // 깊이/높이 (mm)
        double thickness = 0.5; // 종이 두께 (mm)
    };

    /// A형 맞뚜껑 박스 (cross-shaped flat pattern, top lid attached)
    void createABox(const BoxParams& p);

    /// 접어넣기 박스 (tuck-end box, top and bottom tuck flaps)
    void createTuckBox(const BoxParams& p);

    /// 트레이 (simple four-sided tray with corner notches)
    void createTray(const BoxParams& p);

    /// 자동잠금 박스 (auto-lock bottom)
    void createAutoLock(const BoxParams& p);

    /// 선물 박스 — 상하 분리형 뚜껑 (separate lid and base)
    void createGiftBox(const BoxParams& p);

private:
    // --- low-level helpers ---

    /// Add a single knife line entity.
    void addKnifeLine(const Point2D& p1, const Point2D& p2);

    /// Add a single oshi (crease/fold) line entity.
    void addOshiLine(const Point2D& p1, const Point2D& p2);

    /// Add four knife lines forming an axis-aligned rectangle.
    /// (x, y) = bottom-left corner, w = width, h = height.
    void addKnifeRect(double x, double y, double w, double h);

    /// Add four oshi lines forming an axis-aligned rectangle.
    void addOshiRect(double x, double y, double w, double h);

    Document& m_doc;
};

} // namespace yjcad
