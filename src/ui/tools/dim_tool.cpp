#include "dim_tool.h"
#include "core/document/document.h"
#include "ui/cad_canvas.h"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace yjcad {

bool DimLinearTool::mousePress(const Point2D& worldPos, Qt::MouseButton button) {
    if (button == Qt::RightButton) { cancel(); return true; }
    if (button != Qt::LeftButton) return false;

    if (m_clickCount == 0) {
        m_point1 = worldPos;
        m_clickCount = 1;
        return true;
    }

    if (m_clickCount == 1) {
        m_point2 = worldPos;
        m_clickCount = 2;
        return true;
    }

    // Third click: dimension line position (offset from measurement line)
    createDimension(m_point1, m_point2, worldPos);
    m_clickCount = 0;
    m_preview.clear();
    if (m_canvas) m_canvas->refreshEntities();
    return true;
}

bool DimLinearTool::mouseMove(const Point2D& worldPos) {
    m_preview.clear();

    if (m_clickCount == 1) {
        m_preview.push_back({m_point1, worldPos});
    } else if (m_clickCount == 2) {
        // Show dimension preview
        double dx = m_point2.x - m_point1.x;
        double dy = m_point2.y - m_point1.y;
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-9) return false;

        // Determine offset direction from cursor
        double nx = -dy / len;
        double ny = dx / len;
        double offset = (worldPos.x - m_point1.x) * nx + (worldPos.y - m_point1.y) * ny;

        Point2D d1{m_point1.x + nx * offset, m_point1.y + ny * offset};
        Point2D d2{m_point2.x + nx * offset, m_point2.y + ny * offset};

        // Extension lines
        m_preview.push_back({m_point1, d1});
        m_preview.push_back({m_point2, d2});
        // Dimension line
        m_preview.push_back({d1, d2});
    }

    return m_clickCount > 0;
}

void DimLinearTool::cancel() {
    m_clickCount = 0;
    m_preview.clear();
}

void DimLinearTool::createDimension(const Point2D& p1, const Point2D& p2,
                                     const Point2D& dimLinePos) {
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 1e-9) return;

    double nx = -dy / dist;
    double ny = dx / dist;
    double offset = (dimLinePos.x - p1.x) * nx + (dimLinePos.y - p1.y) * ny;

    Point2D d1{p1.x + nx * offset, p1.y + ny * offset};
    Point2D d2{p2.x + nx * offset, p2.y + ny * offset};

    // Add to "DIM" layer
    m_doc->layers().addLayer("DIM", 4); // cyan

    // Extension line 1
    auto ext1 = std::make_unique<LineEntity>(m_doc->nextId(), p1, d1);
    ext1->setLayerName("DIM");
    m_doc->history().execute(std::make_unique<AddEntityCommand>(*m_doc, std::move(ext1)));

    // Extension line 2
    auto ext2 = std::make_unique<LineEntity>(m_doc->nextId(), p2, d2);
    ext2->setLayerName("DIM");
    m_doc->history().execute(std::make_unique<AddEntityCommand>(*m_doc, std::move(ext2)));

    // Dimension line
    auto dimLine = std::make_unique<LineEntity>(m_doc->nextId(), d1, d2);
    dimLine->setLayerName("DIM");
    m_doc->history().execute(std::make_unique<AddEntityCommand>(*m_doc, std::move(dimLine)));

    // Dimension text
    Point2D textPos{(d1.x + d2.x) / 2.0, (d1.y + d2.y) / 2.0 + 2.0};
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << dist;
    auto text = std::make_unique<TextEntity>(m_doc->nextId(), textPos, oss.str(), 3.0);
    text->setLayerName("DIM");
    m_doc->history().execute(std::make_unique<AddEntityCommand>(*m_doc, std::move(text)));

    // Arrow heads (small lines at each end)
    double arrowLen = 2.0;
    double angle = std::atan2(dy, dx);
    for (int side = 0; side < 2; ++side) {
        Point2D base = (side == 0) ? d1 : d2;
        double dir = (side == 0) ? 0.0 : 3.14159265358979323846;
        for (int a = -1; a <= 1; a += 2) {
            double aa = angle + dir + a * 0.3;
            Point2D tip{base.x + arrowLen * std::cos(aa),
                        base.y + arrowLen * std::sin(aa)};
            auto arrow = std::make_unique<LineEntity>(m_doc->nextId(), base, tip);
            arrow->setLayerName("DIM");
            m_doc->history().execute(std::make_unique<AddEntityCommand>(*m_doc, std::move(arrow)));
        }
    }
}

} // namespace yjcad
