#pragma once

#include "core/document/entity.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <string>

namespace yjcad {

class Document;
class CadCanvas;

/// Tool mode for status bar display
enum class ToolMode {
    Select,     // Default: select/move entities
    // Draw
    DrawLine, DrawCircle, DrawArc, DrawPolyline,
    DrawRect, DrawEllipse, DrawPolygon, DrawPoint, DrawText, DrawXLine,
    // Edit
    Move, Copy, Rotate, Mirror, Scale, Offset, Trim, Delete,
    Array, PolarArray, Stretch, Extend, Break, Divide,
    Chamfer, Fillet,
    // Group
    Join, Explode,
    // Inquiry
    Distance, AreaCalc,
    // Dimension
    Dim,
    // Zoom
    ZoomWindow
};

/// Base class for interactive CAD tools
class Tool {
public:
    virtual ~Tool() = default;

    virtual std::string name() const = 0;
    virtual ToolMode mode() const = 0;

    /// Mouse events — return true if handled
    virtual bool mousePress(const Point2D& worldPos, Qt::MouseButton button) = 0;
    virtual bool mouseMove(const Point2D& worldPos) = 0;
    virtual bool mouseRelease(const Point2D& worldPos, Qt::MouseButton button) { (void)worldPos; (void)button; return false; }

    /// Key events
    virtual bool keyPress(int key) { (void)key; return false; }

    /// Cancel current operation (Esc)
    virtual void cancel() {}

    /// Get preview entities for rubber-band display
    virtual const std::vector<std::pair<Point2D, Point2D>>& previewLines() const {
        static std::vector<std::pair<Point2D, Point2D>> empty;
        return empty;
    }

    void setDocument(Document* doc) { m_doc = doc; }
    void setCanvas(CadCanvas* canvas) { m_canvas = canvas; }

protected:
    Document* m_doc = nullptr;
    CadCanvas* m_canvas = nullptr;
};

} // namespace yjcad
