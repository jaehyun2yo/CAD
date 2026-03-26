#include <gtest/gtest.h>
#include "core/document/document.h"

using namespace yjcad;

class CommandTest : public ::testing::Test {
protected:
    Document doc;
};

TEST_F(CommandTest, UndoRedoAddEntity) {
    EntityId id = doc.nextId();
    auto line = std::make_unique<LineEntity>(id, Point2D{0, 0}, Point2D{10, 10});
    auto cmd = std::make_unique<AddEntityCommand>(doc, std::move(line));

    doc.history().execute(std::move(cmd));
    EXPECT_EQ(doc.entityCount(), 1u);
    EXPECT_NE(doc.entity(id), nullptr);

    // Undo
    EXPECT_TRUE(doc.history().undo());
    EXPECT_EQ(doc.entityCount(), 0u);
    EXPECT_EQ(doc.entity(id), nullptr);

    // Redo
    EXPECT_TRUE(doc.history().redo());
    EXPECT_EQ(doc.entityCount(), 1u);
    EXPECT_NE(doc.entity(id), nullptr);
}

TEST_F(CommandTest, UndoRedoRemoveEntity) {
    // First add directly
    EntityId id = doc.nextId();
    doc.addEntity(std::make_unique<LineEntity>(id, Point2D{0, 0}, Point2D{10, 10}));

    // Remove via command
    auto cmd = std::make_unique<RemoveEntityCommand>(doc, id);
    doc.history().execute(std::move(cmd));
    EXPECT_EQ(doc.entityCount(), 0u);

    // Undo restores entity
    EXPECT_TRUE(doc.history().undo());
    EXPECT_EQ(doc.entityCount(), 1u);
    EXPECT_NE(doc.entity(id), nullptr);
}

TEST_F(CommandTest, UndoRedoMove) {
    EntityId id = doc.nextId();
    doc.addEntity(std::make_unique<LineEntity>(id, Point2D{0, 0}, Point2D{10, 0}));

    auto cmd = std::make_unique<MoveCommand>(doc, std::vector<EntityId>{id}, Point2D{5, 3});
    doc.history().execute(std::move(cmd));

    auto* line = dynamic_cast<LineEntity*>(doc.entity(id));
    ASSERT_NE(line, nullptr);
    EXPECT_NEAR(line->start().x, 5.0, 1e-9);
    EXPECT_NEAR(line->start().y, 3.0, 1e-9);

    // Undo
    doc.history().undo();
    EXPECT_NEAR(line->start().x, 0.0, 1e-9);
    EXPECT_NEAR(line->start().y, 0.0, 1e-9);

    // Redo
    doc.history().redo();
    EXPECT_NEAR(line->start().x, 5.0, 1e-9);
}

TEST_F(CommandTest, UndoEmpty) {
    EXPECT_FALSE(doc.history().undo());
}

TEST_F(CommandTest, RedoEmpty) {
    EXPECT_FALSE(doc.history().redo());
}

TEST_F(CommandTest, NewActionClearsRedoStack) {
    EntityId id1 = doc.nextId();
    auto cmd1 = std::make_unique<AddEntityCommand>(
        doc, std::make_unique<LineEntity>(id1, Point2D{0, 0}, Point2D{10, 10}));
    doc.history().execute(std::move(cmd1));

    doc.history().undo();
    EXPECT_TRUE(doc.history().canRedo());

    // New action clears redo
    EntityId id2 = doc.nextId();
    auto cmd2 = std::make_unique<AddEntityCommand>(
        doc, std::make_unique<LineEntity>(id2, Point2D{0, 0}, Point2D{5, 5}));
    doc.history().execute(std::move(cmd2));
    EXPECT_FALSE(doc.history().canRedo());
}

TEST_F(CommandTest, MultipleUndoRedo) {
    // Add 3 entities
    for (int i = 0; i < 3; ++i) {
        EntityId id = doc.nextId();
        auto cmd = std::make_unique<AddEntityCommand>(
            doc, std::make_unique<LineEntity>(id, Point2D{0, 0}, Point2D{10.0 * i, 0}));
        doc.history().execute(std::move(cmd));
    }
    EXPECT_EQ(doc.entityCount(), 3u);

    // Undo all
    doc.history().undo();
    doc.history().undo();
    doc.history().undo();
    EXPECT_EQ(doc.entityCount(), 0u);

    // Redo all
    doc.history().redo();
    doc.history().redo();
    doc.history().redo();
    EXPECT_EQ(doc.entityCount(), 3u);
}
