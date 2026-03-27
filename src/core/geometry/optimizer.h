#pragma once

#include "core/document/entity.h"
#include <vector>
#include <map>

namespace yjcad {

class Document;

/// Optimizer — document-level geometry cleanup and analysis utilities
class Optimizer {
public:
    explicit Optimizer(Document& doc);

    // ── Cleanup ──

    /// Remove zero-length lines and entities smaller than minLen (mm).
    /// Returns the number of entities removed.
    int clean(double minLen = 0.01);

    // ── Overlap detection ──

    /// Find entity IDs that are duplicates or fully overlapping another entity.
    /// Checks position equality within tolerance.
    std::vector<EntityId> findOverlaps(double tolerance = 0.01);

    /// Remove overlapping duplicates from the document.
    /// Returns the number of entities removed.
    int removeOverlaps(double tolerance = 0.01);

    // ── Snap join ──

    /// Connect line endpoints that are within gap (mm) by moving them to the midpoint.
    /// Returns the number of endpoint pairs snapped.
    int snapJoin(double gap = 0.1);

    // ── Measurement ──

    /// Total combined length of the given entities.
    double totalLength(const std::vector<EntityId>& ids);

    /// Combined length grouped by LineClassification.
    std::map<LineClassification, double> lengthByClass(const std::vector<EntityId>& ids);

private:
    Document& m_doc;
};

} // namespace yjcad
