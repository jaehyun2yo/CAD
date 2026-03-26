#include <gtest/gtest.h>
#include "core/document/document.h"

using namespace yjcad;

class DocumentTest : public ::testing::Test {
protected:
    Document doc;

    EntityId addLine(double x1, double y1, double x2, double y2) {
        EntityId id = doc.nextId();
        auto line = std::make_unique<LineEntity>(id, Point2D{x1, y1}, Point2D{x2, y2});
        doc.addEntity(std::move(line));
        return id;
    }
};

TEST_F(DocumentTest, InitiallyEmpty) {
    EXPECT_EQ(doc.entityCount(), 0u);
    EXPECT_FALSE(doc.isModified());
}

TEST_F(DocumentTest, AddEntity) {
    auto id = addLine(0, 0, 10, 10);
    EXPECT_EQ(doc.entityCount(), 1u);
    EXPECT_NE(doc.entity(id), nullptr);
    EXPECT_TRUE(doc.isModified());
}

TEST_F(DocumentTest, RemoveEntity) {
    auto id = addLine(0, 0, 10, 10);
    auto removed = doc.removeEntity(id);
    EXPECT_NE(removed, nullptr);
    EXPECT_EQ(doc.entityCount(), 0u);
    EXPECT_EQ(doc.entity(id), nullptr);
}

TEST_F(DocumentTest, RemoveNonExistent) {
    auto removed = doc.removeEntity(999);
    EXPECT_EQ(removed, nullptr);
}

TEST_F(DocumentTest, EntitiesInBox) {
    addLine(0, 0, 5, 5);     // inside box
    addLine(20, 20, 25, 25); // outside box
    addLine(8, 8, 12, 12);   // partially inside

    BoundingBox box{{0, 0}, {10, 10}};
    auto ids = doc.entitiesInBox(box);
    EXPECT_EQ(ids.size(), 2u); // first and third overlap with box
}

TEST_F(DocumentTest, Bounds) {
    addLine(-10, -5, 20, 15);
    addLine(0, 0, 5, 5);

    auto bb = doc.bounds();
    EXPECT_DOUBLE_EQ(bb.min.x, -10.0);
    EXPECT_DOUBLE_EQ(bb.min.y, -5.0);
    EXPECT_DOUBLE_EQ(bb.max.x, 20.0);
    EXPECT_DOUBLE_EQ(bb.max.y, 15.0);
}

TEST_F(DocumentTest, Clear) {
    addLine(0, 0, 10, 10);
    addLine(5, 5, 15, 15);
    doc.selection().select(1);

    doc.clear();
    EXPECT_EQ(doc.entityCount(), 0u);
    EXPECT_TRUE(doc.selection().empty());
    EXPECT_FALSE(doc.isModified());
}

// ── Layer tests ──

TEST_F(DocumentTest, DefaultLayerExists) {
    EXPECT_NE(doc.layers().layer("0"), nullptr);
    EXPECT_EQ(doc.layers().currentLayer(), "0");
}

TEST_F(DocumentTest, AddLayer) {
    EXPECT_TRUE(doc.layers().addLayer("cut", 1));
    EXPECT_TRUE(doc.layers().addLayer("oshi", 3));
    EXPECT_EQ(doc.layers().count(), 3u); // "0" + "cut" + "oshi"
}

TEST_F(DocumentTest, CannotRemoveDefaultLayer) {
    EXPECT_FALSE(doc.layers().removeLayer("0"));
}

TEST_F(DocumentTest, CannotRemoveCurrentLayer) {
    doc.layers().addLayer("test");
    doc.layers().setCurrentLayer("test");
    EXPECT_FALSE(doc.layers().removeLayer("test"));
}

// ── Selection tests ──

TEST_F(DocumentTest, SelectEntity) {
    auto id = addLine(0, 0, 10, 10);
    doc.selection().select(id);
    EXPECT_TRUE(doc.selection().isSelected(id));
    EXPECT_EQ(doc.selection().count(), 1u);
}

TEST_F(DocumentTest, DeselectOnRemove) {
    auto id = addLine(0, 0, 10, 10);
    doc.selection().select(id);
    doc.removeEntity(id);
    EXPECT_FALSE(doc.selection().isSelected(id));
}

TEST_F(DocumentTest, ToggleSelection) {
    auto id = addLine(0, 0, 10, 10);
    doc.selection().toggle(id);
    EXPECT_TRUE(doc.selection().isSelected(id));
    doc.selection().toggle(id);
    EXPECT_FALSE(doc.selection().isSelected(id));
}
