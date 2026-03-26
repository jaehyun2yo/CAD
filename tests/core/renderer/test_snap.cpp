#include <gtest/gtest.h>
#include "core/renderer/snap.h"

using namespace yjcad;

class SnapTest : public ::testing::Test {
protected:
    Document doc;

    void SetUp() override {
        // Add some entities for snap targets
        auto id1 = doc.nextId();
        doc.addEntity(std::make_unique<LineEntity>(id1, Point2D{0, 0}, Point2D{100, 0}));

        auto id2 = doc.nextId();
        doc.addEntity(std::make_unique<CircleEntity>(id2, Point2D{50, 50}, 20.0));

        auto id3 = doc.nextId();
        doc.addEntity(std::make_unique<LineEntity>(id3, Point2D{0, 0}, Point2D{0, 100}));
    }
};

TEST_F(SnapTest, GridSnap) {
    SnapEngine snap(doc);
    snap.setGridSnap(true, 10.0);
    snap.setEndpointSnap(false);
    snap.setMidpointSnap(false);
    snap.setCenterSnap(false);

    auto result = snap.snap({12.3, 7.8}, 15.0);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, SnapType::Grid);
    EXPECT_NEAR(result.point.x, 10.0, 0.01);
    EXPECT_NEAR(result.point.y, 10.0, 0.01);
}

TEST_F(SnapTest, EndpointSnap) {
    SnapEngine snap(doc);
    auto result = snap.snap({1.5, 0.5}, 5.0);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, SnapType::Endpoint);
    EXPECT_NEAR(result.point.x, 0.0, 0.01);
    EXPECT_NEAR(result.point.y, 0.0, 0.01);
}

TEST_F(SnapTest, MidpointSnap) {
    SnapEngine snap(doc);
    auto result = snap.snap({49.5, 0.5}, 5.0);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, SnapType::Midpoint);
    EXPECT_NEAR(result.point.x, 50.0, 0.01);
    EXPECT_NEAR(result.point.y, 0.0, 0.01);
}

TEST_F(SnapTest, CenterSnap) {
    SnapEngine snap(doc);
    auto result = snap.snap({51.0, 49.0}, 5.0);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, SnapType::Center);
    EXPECT_NEAR(result.point.x, 50.0, 0.01);
    EXPECT_NEAR(result.point.y, 50.0, 0.01);
}

TEST_F(SnapTest, NoSnapOutOfRange) {
    SnapEngine snap(doc);
    snap.setGridSnap(false); // disable grid to test entity snaps only
    auto result = snap.snap({500.0, 500.0}, 5.0);
    EXPECT_FALSE(result.valid);
}

TEST_F(SnapTest, EndpointPriorityOverGrid) {
    SnapEngine snap(doc);
    snap.setGridSnap(true, 10.0);
    // Point near origin (0,0) which is an endpoint — should snap to endpoint, not grid
    auto result = snap.snap({0.5, 0.5}, 5.0);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.type, SnapType::Endpoint);
}
