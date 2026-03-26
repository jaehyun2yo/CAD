#include <gtest/gtest.h>
#include "core/geometry/operations.h"
#include <cmath>

using namespace yjcad;

static constexpr double PI = 3.14159265358979323846;

// ── Bridge insertion tests ──

TEST(BridgeInsert, ShortLine) {
    LineEntity line(1, {0, 0}, {5, 0});
    auto segs = GeometryOps::insertBridges(line, 2.0, 100.0, 100);
    // Line shorter than spacing → no bridges, returned as-is
    EXPECT_EQ(segs.size(), 1u);
}

TEST(BridgeInsert, NormalLine) {
    LineEntity line(1, {0, 0}, {100, 0});
    auto segs = GeometryOps::insertBridges(line, 3.0, 20.0, 100);
    // Should have multiple segments with gaps
    EXPECT_GT(segs.size(), 1u);

    // Total line length should be less than original (bridges removed)
    double totalLen = 0;
    for (const auto& seg : segs) totalLen += seg->length();
    EXPECT_LT(totalLen, 100.0);
    EXPECT_GT(totalLen, 50.0); // at least half should remain
}

TEST(BridgeInsert, PreservesClassification) {
    LineEntity line(1, {0, 0}, {100, 0});
    line.setClassification(LineClassification::Knife);
    line.setLayerName("cut");

    auto segs = GeometryOps::insertBridges(line, 3.0, 20.0, 100);
    for (const auto& seg : segs) {
        EXPECT_EQ(seg->classification(), LineClassification::Knife);
        EXPECT_EQ(seg->layerName(), "cut");
    }
}

// ── Arc to polyline tests (점빼기) ──

TEST(ArcToPolyline, FullArc) {
    ArcEntity arc(1, {0, 0}, 10.0, 0.0, 180.0);
    auto poly = GeometryOps::arcToPolyline(arc, 0.1, 100);

    ASSERT_NE(poly, nullptr);
    EXPECT_GT(poly->vertices().size(), 4u);

    // First point should be at (10, 0) — start of arc at 0 degrees
    EXPECT_NEAR(poly->vertices().front().x, 10.0, 0.2);
    EXPECT_NEAR(poly->vertices().front().y, 0.0, 0.2);

    // Last point should be at (-10, 0) — end at 180 degrees
    EXPECT_NEAR(poly->vertices().back().x, -10.0, 0.2);
    EXPECT_NEAR(poly->vertices().back().y, 0.0, 0.2);
}

TEST(ArcToPolyline, Precision) {
    ArcEntity arc(1, {0, 0}, 50.0, 0.0, 90.0);
    auto poly = GeometryOps::arcToPolyline(arc, 0.01, 100); // 0.01mm tolerance

    // All vertices should be within tolerance of the arc
    for (const auto& v : poly->vertices()) {
        double dist = std::sqrt(v.x * v.x + v.y * v.y);
        EXPECT_NEAR(dist, 50.0, 0.02); // within 2x tolerance
    }
}

TEST(ArcToPolyline, PreservesProperties) {
    ArcEntity arc(1, {5, 5}, 10.0, 0.0, 360.0);
    arc.setClassification(LineClassification::Oshi);
    arc.setLayerName("fold");

    auto poly = GeometryOps::arcToPolyline(arc, 0.1, 100);
    EXPECT_EQ(poly->classification(), LineClassification::Oshi);
    EXPECT_EQ(poly->layerName(), "fold");
}

// ── Circle to polyline ──

TEST(CircleToPolyline, Basic) {
    CircleEntity circle(1, {0, 0}, 10.0);
    auto poly = GeometryOps::circleToPolyline(circle, 64, 100);

    EXPECT_EQ(poly->vertices().size(), 64u);
    EXPECT_TRUE(poly->isClosed());

    // All vertices on circle
    for (const auto& v : poly->vertices()) {
        double dist = std::sqrt(v.x * v.x + v.y * v.y);
        EXPECT_NEAR(dist, 10.0, 0.01);
    }
}

// ── Offset polyline ──

TEST(OffsetPolyline, SquareExpand) {
    // Square 10x10, offset outward by 2
    PolylineEntity square(1, {{0,0}, {10,0}, {10,10}, {0,10}}, true);
    auto result = GeometryOps::offsetPolyline(square, 2.0, 100);

    ASSERT_NE(result, nullptr);
    // Expanded square should be larger
    auto bb = result->boundingBox();
    EXPECT_LT(bb.min.x, -1.0); // should extend beyond original
    EXPECT_GT(bb.max.x, 11.0);
}

TEST(OffsetPolyline, SquareShrink) {
    PolylineEntity square(1, {{0,0}, {10,0}, {10,10}, {0,10}}, true);
    auto result = GeometryOps::offsetPolyline(square, -2.0, 100);

    ASSERT_NE(result, nullptr);
    auto bb = result->boundingBox();
    EXPECT_GT(bb.min.x, 1.0);
    EXPECT_LT(bb.max.x, 9.0);
}

// ── Point to segment distance ──

TEST(PointToSegment, OnLine) {
    double d = GeometryOps::pointToSegmentDistance({5, 0}, {0, 0}, {10, 0});
    EXPECT_NEAR(d, 0.0, 1e-9);
}

TEST(PointToSegment, Perpendicular) {
    double d = GeometryOps::pointToSegmentDistance({5, 3}, {0, 0}, {10, 0});
    EXPECT_NEAR(d, 3.0, 1e-9);
}

TEST(PointToSegment, BeyondEnd) {
    double d = GeometryOps::pointToSegmentDistance({15, 0}, {0, 0}, {10, 0});
    EXPECT_NEAR(d, 5.0, 1e-9);
}

TEST(PointToSegment, ZeroLength) {
    double d = GeometryOps::pointToSegmentDistance({3, 4}, {0, 0}, {0, 0});
    EXPECT_NEAR(d, 5.0, 1e-9);
}
