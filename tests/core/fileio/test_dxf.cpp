#include <gtest/gtest.h>
#include "core/fileio/dxf_reader.h"
#include "core/fileio/dxf_writer.h"
#include <filesystem>
#include <fstream>

using namespace yjcad;
namespace fs = std::filesystem;

static const std::string TEST_DIR = "test_output";

class DxfTest : public ::testing::Test {
protected:
    Document doc;

    void SetUp() override {
        fs::create_directories(TEST_DIR);
    }

    void TearDown() override {
        fs::remove_all(TEST_DIR);
    }

    // Create a simple DXF file manually for testing
    std::string createSimpleDxf() {
        std::string path = TEST_DIR + "/simple.dxf";
        std::ofstream f(path);
        f << "  0\nSECTION\n  2\nHEADER\n  0\nENDSEC\n";
        f << "  0\nSECTION\n  2\nTABLES\n";
        // Layer table
        f << "  0\nTABLE\n  2\nLAYER\n 70\n2\n";
        f << "  0\nLAYER\n  2\n0\n 70\n0\n 62\n7\n  6\nCONTINUOUS\n";
        f << "  0\nLAYER\n  2\ncut\n 70\n0\n 62\n1\n  6\nCONTINUOUS\n";
        f << "  0\nENDTAB\n";
        f << "  0\nENDSEC\n";
        // Entities
        f << "  0\nSECTION\n  2\nENTITIES\n";
        // Line
        f << "  0\nLINE\n  8\n0\n 10\n0.0\n 20\n0.0\n 11\n100.0\n 21\n50.0\n";
        // Circle
        f << "  0\nCIRCLE\n  8\ncut\n 10\n50.0\n 20\n25.0\n 40\n10.0\n";
        // Another line
        f << "  0\nLINE\n  8\ncut\n 10\n10.0\n 20\n10.0\n 11\n90.0\n 21\n40.0\n";
        f << "  0\nENDSEC\n  0\nEOF\n";
        f.close();
        return path;
    }
};

TEST_F(DxfTest, LoadNonExistentFile) {
    DxfReader reader(doc);
    auto result = reader.load("nonexistent.dxf");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST_F(DxfTest, LoadSimpleDxf) {
    std::string path = createSimpleDxf();
    DxfReader reader(doc);
    auto result = reader.load(path);

    EXPECT_TRUE(result.success) << result.errorMessage;
    EXPECT_EQ(result.entityCount, 3u); // 1 line + 1 circle + 1 line
    EXPECT_GE(result.layerCount, 2u);  // "0" + "cut"
}

TEST_F(DxfTest, LoadedEntitiesHaveCorrectTypes) {
    createSimpleDxf();
    DxfReader reader(doc);
    reader.load(TEST_DIR + "/simple.dxf");

    int lines = 0, circles = 0;
    for (auto id : doc.entityIds()) {
        auto* e = doc.entity(id);
        if (e->type() == EntityType::Line) ++lines;
        if (e->type() == EntityType::Circle) ++circles;
    }
    EXPECT_EQ(lines, 2);
    EXPECT_EQ(circles, 1);
}

TEST_F(DxfTest, LoadedLayersExist) {
    createSimpleDxf();
    DxfReader reader(doc);
    reader.load(TEST_DIR + "/simple.dxf");

    EXPECT_NE(doc.layers().layer("0"), nullptr);
    EXPECT_NE(doc.layers().layer("cut"), nullptr);
}

TEST_F(DxfTest, SaveAndReload) {
    // Create document with entities
    auto id1 = doc.nextId();
    doc.addEntity(std::make_unique<LineEntity>(id1, Point2D{0, 0}, Point2D{100, 50}));

    auto id2 = doc.nextId();
    doc.addEntity(std::make_unique<CircleEntity>(id2, Point2D{50, 25}, 15.0));

    auto id3 = doc.nextId();
    doc.addEntity(std::make_unique<PolylineEntity>(id3,
        std::vector<Point2D>{{0,0}, {10,0}, {10,10}, {0,10}}, true));

    doc.layers().addLayer("test_layer", 3);

    // Save
    std::string savePath = TEST_DIR + "/roundtrip.dxf";
    DxfWriter writer(doc);
    auto saveResult = writer.save(savePath);
    EXPECT_TRUE(saveResult.success) << saveResult.errorMessage;
    EXPECT_TRUE(fs::exists(savePath));
    EXPECT_FALSE(fs::exists(savePath + ".tmp")); // temp file should be gone

    // Reload into new document
    Document doc2;
    DxfReader reader(doc2);
    auto loadResult = reader.load(savePath);
    EXPECT_TRUE(loadResult.success) << loadResult.errorMessage;
    EXPECT_EQ(loadResult.entityCount, 3u);
}

TEST_F(DxfTest, AtomicSaveNoLeftoverTemp) {
    auto id = doc.nextId();
    doc.addEntity(std::make_unique<LineEntity>(id, Point2D{0, 0}, Point2D{10, 10}));

    std::string savePath = TEST_DIR + "/atomic_test.dxf";
    DxfWriter writer(doc);
    writer.save(savePath);

    // Verify: no .tmp file left behind
    EXPECT_FALSE(fs::exists(savePath + ".tmp"));
    EXPECT_TRUE(fs::exists(savePath));
}

TEST_F(DxfTest, GenerateFilename) {
    auto name = DxfWriter::generateFilename(3, 26, 1, "원컴퍼니", "퀘이크업미니가차", "2절");
    EXPECT_EQ(name, "0326-1 원컴퍼니 퀘이크업미니가차 2절.DXF");
}

TEST_F(DxfTest, GenerateFilenameWithPadding) {
    auto name = DxfWriter::generateFilename(1, 5, 12, "대성목형", "테스트", "4절");
    EXPECT_EQ(name, "0105-12 대성목형 테스트 4절.DXF");
}
