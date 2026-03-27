#pragma once

#include "core/document/entity.h"
#include <vector>
#include <memory>

namespace yjcad {

class Document;

/// CadClipboard — in-process clipboard for copy/paste of CAD entities
class CadClipboard {
public:
    CadClipboard() = default;

    /// Copy selected entities from the document.
    /// Clones each selected entity and records the centroid as base point.
    void copy(const Document& doc);

    /// Paste previously copied entities into the document, translated to basePoint.
    /// Uses AddEntityCommand for each entity (Undo/Redo compatible).
    void paste(Document& doc, const Point2D& basePoint);

    /// Returns true if there is data ready to paste.
    bool hasData() const { return !m_entities.empty(); }

    /// Number of entities stored in the clipboard.
    size_t count() const { return m_entities.size(); }

    /// Clear clipboard contents.
    void clear();

private:
    /// Compute the bounding-box centroid of the stored entities.
    Point2D computeCentroid() const;

    std::vector<std::unique_ptr<Entity>> m_entities;
    Point2D m_basePoint; // centroid of copied entities in their original coordinate space
};

} // namespace yjcad
