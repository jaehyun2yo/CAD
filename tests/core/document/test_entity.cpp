#include <gtest/gtest.h>
#include "core/document/entity.h"

using namespace yjcad;

// ── Point2D tests ──

TEST(Point2D, DefaultConstruction) {
    Point2D p;
    EXPECT_DOUBLE_EQ(p.x, 0.0);
    EXPECT_DOUBLE_EQ(p.y, 0.0);
}

TEST(Point2D, Distance) {
    Point2D a(0, 0);
    Point2D b(3, 4);
    EXPECT_DOUBLE_EQ(a.distanceTo(b), 5.0);
}

TEST(Point2D, Arithmetic) {
    Point2D a(1, 2);
    Point2D b(3, 4);
    auto sum = a + b;
    EXPECT_DOUBLE_EQ(sum.x, 4.0);
    EXPECT_DOUBLE_EQ(sum.y, 6.0);
}

// ── BoundingBox tests ──

TEST(BoundingBox, Contains) {
    BoundingBox bb{{0, 0}, {10, 10}};
    EXPECT_TRUE(bb.contains({5, 5}));
    EXPECT_TRUE(bb.contains({0, 0}));
    EXPECT_FALSE(bb.contains({-1, 5}));
    EXPECT_FALSE(bb.contains({11, 5}));
}

TEST(BoundingBox, Intersects) {
    BoundingBox a{{0, 0}, {10, 10}};
    BoundingBox b{{5, 5}, {15, 15}};
    BoundingBox c{{20, 20}, {30, 30}};
    EXPECT_TRUE(a.intersects(b));
    EXPECT_FALSE(a.intersects(c));
}

// ── LineEntity tests ──

TEST(LineEntity, Construction) {
    LineEntity line(1, {0, 0}, {10, 0});
    EXPECT_EQ(line.id(), 1);
    EXPECT_EQ(line.type(), EntityType::Line);
    EXPECT_DOUBLE_EQ(line.length(), 10.0);
}

TEST(LineEntity, BoundingBox) {
    LineEntity line(1, {-5, 3}, {10, -2});
    auto bb = line.boundingBox();
    EXPECT_DOUBLE_EQ(bb.min.x, -5.0);
    EXPECT_DOUBLE_EQ(bb.min.y, -2.0);
    EXPECT_DOUBLE_EQ(bb.max.x, 10.0);
    EXPECT_DOUBLE_EQ(bb.max.y, 3.0);
}

TEST(LineEntity, Translate) {
    LineEntity line(1, {0, 0}, {10, 0});
    line.translate({5, 3});
    EXPECT_EQ(line.start(), Point2D(5, 3));
    EXPECT_EQ(line.end(), Point2D(15, 3));
}

TEST(LineEntity, Rotate90) {
    LineEntity line(1, {10, 0}, {10, 0});
    // Rotate point (10,0) around origin by 90 degrees
    LineEntity line2(2, {10, 0}, {20, 0});
    line2.rotate({0, 0}, 90.0);
    EXPECT_NEAR(line2.start().x, 0.0, 1e-9);
    EXPECT_NEAR(line2.start().y, 10.0, 1e-9);
}

TEST(LineEntity, Clone) {
    LineEntity line(1, {0, 0}, {10, 5});
    line.setLayerName("cut");
    line.setColor(1);
    line.setClassification(LineClassification::Knife);

    auto cloned = line.clone();
    auto* clonedLine = dynamic_cast<LineEntity*>(cloned.get());
    ASSERT_NE(clonedLine, nullptr);
    EXPECT_EQ(clonedLine->start(), Point2D(0, 0));
    EXPECT_EQ(clonedLine->end(), Point2D(10, 5));
    EXPECT_EQ(clonedLine->layerName(), "cut");
    EXPECT_EQ(clonedLine->classification(), LineClassification::Knife);
}

TEST(LineEntity, MirrorHorizontal) {
    // Mirror across X axis (y=0 line)
    LineEntity line(1, {5, 3}, {10, 7});
    line.mirror({0, 0}, {1, 0});
    EXPECT_NEAR(line.start().x, 5.0, 1e-9);
    EXPECT_NEAR(line.start().y, -3.0, 1e-9);
    EXPECT_NEAR(line.end().x, 10.0, 1e-9);
    EXPECT_NEAR(line.end().y, -7.0, 1e-9);
}

TEST(LineEntity, Scale) {
    LineEntity line(1, {10, 0}, {20, 0});
    line.scale({0, 0}, 2.0);
    EXPECT_NEAR(line.start().x, 20.0, 1e-9);
    EXPECT_NEAR(line.end().x, 40.0, 1e-9);
}

// ── CircleEntity tests ──

TEST(CircleEntity, BoundingBox) {
    CircleEntity circle(1, {5, 5}, 3);
    auto bb = circle.boundingBox();
    EXPECT_DOUBLE_EQ(bb.min.x, 2.0);
    EXPECT_DOUBLE_EQ(bb.min.y, 2.0);
    EXPECT_DOUBLE_EQ(bb.max.x, 8.0);
    EXPECT_DOUBLE_EQ(bb.max.y, 8.0);
}

TEST(CircleEntity, ScaleRadius) {
    CircleEntity circle(1, {0, 0}, 5);
    circle.scale({0, 0}, 2.0);
    EXPECT_DOUBLE_EQ(circle.radius(), 10.0);
}

// ── PolylineEntity tests ──

TEST(PolylineEntity, BoundingBox) {
    PolylineEntity poly(1, {{0, 0}, {10, 5}, {5, 10}}, true);
    auto bb = poly.boundingBox();
    EXPECT_DOUBLE_EQ(bb.min.x, 0.0);
    EXPECT_DOUBLE_EQ(bb.min.y, 0.0);
    EXPECT_DOUBLE_EQ(bb.max.x, 10.0);
    EXPECT_DOUBLE_EQ(bb.max.y, 10.0);
}

TEST(PolylineEntity, TranslateAll) {
    PolylineEntity poly(1, {{0, 0}, {10, 0}, {10, 10}});
    poly.translate({5, 5});
    EXPECT_EQ(poly.vertices()[0], Point2D(5, 5));
    EXPECT_EQ(poly.vertices()[1], Point2D(15, 5));
    EXPECT_EQ(poly.vertices()[2], Point2D(15, 15));
}

// ── LineClassification tests ──

TEST(LineClassification, DefaultIsUnknown) {
    LineEntity line(1, {0, 0}, {10, 0});
    EXPECT_EQ(line.classification(), LineClassification::Unknown);
}

TEST(LineClassification, SetOshi) {
    LineEntity line(1, {0, 0}, {10, 0});
    line.setClassification(LineClassification::Oshi);
    EXPECT_EQ(line.classification(), LineClassification::Oshi);
}

TEST(LineClassification, SetKnife) {
    LineEntity line(1, {0, 0}, {10, 0});
    line.setClassification(LineClassification::Knife);
    EXPECT_EQ(line.classification(), LineClassification::Knife);
}
