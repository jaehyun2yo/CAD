#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDockWidget>
#include <QLabel>
#include <QSurfaceFormat>
#include <QKeySequence>

#include "core/document/document.h"
#include "core/fileio/dxf_reader.h"
#include "core/fileio/dxf_writer.h"
#include "core/geometry/operations.h"
#include "ui/cad_canvas.h"
#include "ui/tool_manager.h"
#include "ui/domusong_panel.h"

using namespace yjcad;

int main(int argc, char* argv[]) {
    // Request OpenGL 3.3 Core
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4); // MSAA
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    app.setApplicationName("YJ-CAD");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("YJLaser");

    // Dark theme
    app.setStyleSheet(R"(
        QMainWindow { background-color: #2b2b2b; }
        QMenuBar { background-color: #333333; color: #cccccc; }
        QMenuBar::item:selected { background-color: #4a90d9; }
        QMenu { background-color: #333333; color: #cccccc; }
        QMenu::item:selected { background-color: #4a90d9; }
        QStatusBar { background-color: #252525; color: #aaaaaa; }
        QDockWidget { color: #cccccc; }
        QDockWidget::title { background-color: #333333; padding: 4px; }
    )");

    Document doc;
    QMainWindow window;
    window.setWindowTitle("YJ-CAD v0.1.0 — 도무송 목형 전용 CAD");
    window.resize(1400, 900);

    // ── OpenGL Canvas (center) ──
    auto* canvas = new CadCanvas(doc);
    window.setCentralWidget(canvas);

    // ── 도무송 Panel (Right dock) ──
    auto* domusongPanel = new DomusongPanel(doc, canvas);
    auto* dockWidget = new QDockWidget("도무송", &window);
    dockWidget->setWidget(domusongPanel);
    dockWidget->setMinimumWidth(260);
    dockWidget->setStyleSheet("QDockWidget { color: #cccccc; }");
    window.addDockWidget(Qt::RightDockWidgetArea, dockWidget);

    // (pipeline signal connected after helper lambdas below)

    // ── Status bar ──
    auto* coordLabel = new QLabel("좌표: (0.00, 0.00)");
    auto* layerLabel = new QLabel("레이어: 0");
    auto* entityCountLabel = new QLabel("엔티티: 0");
    coordLabel->setStyleSheet("color: #aaaaaa; padding: 0 8px;");
    layerLabel->setStyleSheet("color: #aaaaaa; padding: 0 8px;");
    entityCountLabel->setStyleSheet("color: #aaaaaa; padding: 0 8px;");
    window.statusBar()->addWidget(coordLabel);
    window.statusBar()->addWidget(layerLabel);
    window.statusBar()->addPermanentWidget(entityCountLabel);

    QObject::connect(canvas, &CadCanvas::cursorWorldPos,
                     [coordLabel](double x, double y) {
        coordLabel->setText(QString("좌표: (%1, %2)")
                            .arg(x, 0, 'f', 2).arg(y, 0, 'f', 2));
    });

    // ── Helper lambdas ──
    auto updateTitle = [&]() {
        QString title = "YJ-CAD v0.1.0";
        if (!doc.filePath().empty()) {
            title += " — " + QString::fromStdString(doc.filePath());
        }
        if (doc.isModified()) title += " *";
        window.setWindowTitle(title);
    };

    auto updateEntityCount = [&]() {
        entityCountLabel->setText(QString("엔티티: %1").arg(doc.entityCount()));
    };

    // Connect domusong panel signal (after helpers are defined)
    QObject::connect(domusongPanel, &DomusongPanel::pipelineCompleted, [&]() {
        updateEntityCount();
        updateTitle();
    });

    // ── File menu ──
    auto* fileMenu = window.menuBar()->addMenu("파일(&F)");

    fileMenu->addAction("열기(&O)", [&]() {
        QString path = QFileDialog::getOpenFileName(&window, "DXF 파일 열기",
            QString(), "DXF 파일 (*.dxf);;모든 파일 (*)");
        if (path.isEmpty()) return;

        DxfReader reader(doc);
        auto result = reader.load(path.toStdString());
        if (!result.success) {
            QMessageBox::critical(&window, "오류",
                QString::fromStdString("DXF 로드 실패: " + result.errorMessage));
            return;
        }

        canvas->refreshEntities();
        canvas->zoomToFit();
        updateTitle();
        updateEntityCount();

        window.statusBar()->showMessage(
            QString("%1 로드 완료 — %2개 엔티티, %3개 레이어")
            .arg(path).arg(result.entityCount).arg(result.layerCount), 5000);
    }, QKeySequence("Ctrl+O"));

    fileMenu->addAction("저장(&S)", [&]() {
        QString path;
        if (doc.filePath().empty()) {
            path = QFileDialog::getSaveFileName(&window, "DXF 파일 저장",
                QString(), "DXF 파일 (*.dxf)");
            if (path.isEmpty()) return;
        } else {
            path = QString::fromStdString(doc.filePath());
        }

        DxfWriter writer(doc);
        auto result = writer.save(path.toStdString());
        if (!result.success) {
            QMessageBox::critical(&window, "오류",
                QString::fromStdString("저장 실패: " + result.errorMessage));
            return;
        }

        doc.setFilePath(path.toStdString());
        doc.setModified(false);
        updateTitle();
        window.statusBar()->showMessage("저장 완료", 3000);
    }, QKeySequence("Ctrl+S"));

    fileMenu->addSeparator();
    fileMenu->addAction("종료(&X)", &app, &QApplication::quit, QKeySequence("Alt+F4"));

    // ── Edit menu ──
    auto* editMenu = window.menuBar()->addMenu("편집(&E)");

    editMenu->addAction("실행 취소(&Z)", [&]() {
        if (doc.history().undo()) {
            canvas->refreshEntities();
            updateTitle();
        }
    }, QKeySequence("Ctrl+Z"));

    editMenu->addAction("다시 실행(&Y)", [&]() {
        if (doc.history().redo()) {
            canvas->refreshEntities();
            updateTitle();
        }
    }, QKeySequence("Ctrl+Y"));

    // ── View menu ──
    auto* viewMenu = window.menuBar()->addMenu("보기(&V)");
    viewMenu->addAction("줌 투 핏(&F)", [canvas]() {
        canvas->zoomToFit();
    }, QKeySequence("Home"));

    // ── Draw menu ──
    auto* drawMenu = window.menuBar()->addMenu("그리기(&D)");
    auto setTool = [&](ToolMode mode, const QString& name) {
        canvas->toolManager().setTool(mode);
        window.statusBar()->showMessage("도구: " + name, 3000);
    };

    drawMenu->addAction("선(&L)", [&]() { setTool(ToolMode::DrawLine, "선 그리기"); }, QKeySequence("L"));
    drawMenu->addAction("원(&C)", [&]() { setTool(ToolMode::DrawCircle, "원 그리기"); }, QKeySequence("C"));

    // ── Edit menu additions ──
    auto* modifyMenu = window.menuBar()->addMenu("수정(&M)");
    modifyMenu->addAction("삭제(&D)", [&]() {
        // Delete selected entities
        auto selected = doc.selection().selectedIds();
        for (EntityId id : selected) {
            auto cmd = std::make_unique<RemoveEntityCommand>(doc, id);
            doc.history().execute(std::move(cmd));
        }
        doc.selection().clear();
        canvas->refreshEntities();
        updateEntityCount();
        updateTitle();
    }, QKeySequence("Delete"));

    modifyMenu->addAction("이동(&M)", [&]() { setTool(ToolMode::Move, "이동"); }, QKeySequence("M"));

    // ── 도무송 menu (핵심!) ──
    auto* domusongMenu = window.menuBar()->addMenu("도무송(&U)");

    domusongMenu->addAction("브릿지 삽입(&B)", [&]() {
        auto selected = doc.selection().selectedIds();
        if (selected.empty()) {
            QMessageBox::information(&window, "도무송", "먼저 칼선을 선택하세요.");
            return;
        }
        int created = 0;
        for (EntityId id : selected) {
            auto* entity = doc.entity(id);
            if (!entity || entity->type() != EntityType::Line) continue;
            auto* line = static_cast<LineEntity*>(entity);

            auto bridgeSegs = GeometryOps::insertBridges(*line, 3.0, 20.0, doc.nextId());
            // Remove original, add bridge segments
            auto rmCmd = std::make_unique<RemoveEntityCommand>(doc, id);
            doc.history().execute(std::move(rmCmd));
            for (auto& seg : bridgeSegs) {
                seg->setClassification(LineClassification::Knife);
                auto addCmd = std::make_unique<AddEntityCommand>(doc, std::move(seg));
                doc.history().execute(std::move(addCmd));
                ++created;
            }
        }
        doc.selection().clear();
        canvas->refreshEntities();
        updateEntityCount();
        window.statusBar()->showMessage(
            QString("브릿지 삽입 완료 — %1개 세그먼트 생성").arg(created), 5000);
    }, QKeySequence("F7"));

    domusongMenu->addAction("점빼기 (호→선분)(&A)", [&]() {
        auto selected = doc.selection().selectedIds();
        if (selected.empty()) {
            QMessageBox::information(&window, "도무송", "먼저 호(Arc)를 선택하세요.");
            return;
        }
        int converted = 0;
        for (EntityId id : selected) {
            auto* entity = doc.entity(id);
            if (!entity || entity->type() != EntityType::Arc) continue;
            auto* arc = static_cast<ArcEntity*>(entity);

            auto poly = GeometryOps::arcToPolyline(*arc, 0.1, doc.nextId());
            if (!poly) continue;

            auto rmCmd = std::make_unique<RemoveEntityCommand>(doc, id);
            doc.history().execute(std::move(rmCmd));
            auto addCmd = std::make_unique<AddEntityCommand>(doc, std::move(poly));
            doc.history().execute(std::move(addCmd));
            ++converted;
        }
        doc.selection().clear();
        canvas->refreshEntities();
        updateEntityCount();
        window.statusBar()->showMessage(
            QString("점빼기 완료 — %1개 호 변환").arg(converted), 5000);
    }, QKeySequence("F8"));

    domusongMenu->addSeparator();

    domusongMenu->addAction("오시로 분류(&O)", [&]() {
        for (EntityId id : doc.selection().selectedIds()) {
            if (auto* e = doc.entity(id))
                e->setClassification(LineClassification::Oshi);
        }
        canvas->refreshEntities();
        window.statusBar()->showMessage("선택된 엔티티를 오시(빨강)로 분류", 3000);
    });

    domusongMenu->addAction("칼로 분류(&K)", [&]() {
        for (EntityId id : doc.selection().selectedIds()) {
            if (auto* e = doc.entity(id))
                e->setClassification(LineClassification::Knife);
        }
        canvas->refreshEntities();
        window.statusBar()->showMessage("선택된 엔티티를 칼(흰색)로 분류", 3000);
    });

    domusongMenu->addSeparator();

    domusongMenu->addAction("구아이선 삽입(&G)", [&]() {
        auto bounds = doc.bounds();
        // Insert guide line 10mm below the bottom of the drawing
        double y = bounds.min.y - 10.0;
        EntityId id = doc.nextId();
        auto guide = std::make_unique<LineEntity>(id,
            Point2D{bounds.min.x, y}, Point2D{bounds.max.x, y});
        guide->setClassification(LineClassification::Guide);
        guide->setLayerName("guide");
        doc.layers().addLayer("guide", 2);
        auto cmd = std::make_unique<AddEntityCommand>(doc, std::move(guide));
        doc.history().execute(std::move(cmd));
        canvas->refreshEntities();
        updateEntityCount();
        window.statusBar()->showMessage("구아이선 삽입 완료 (하단 10mm)", 5000);
    }, QKeySequence("F9"));

    // ── Tool/Help menus ──
    auto* toolMenu = window.menuBar()->addMenu("도구(&T)");
    toolMenu->addAction("선택 모드(&S)", [&]() {
        canvas->toolManager().cancelToSelect();
        window.statusBar()->showMessage("도구: 선택", 2000);
    }, QKeySequence("Escape"));

    auto* helpMenu = window.menuBar()->addMenu("도움말(&H)");
    helpMenu->addAction("정보(&A)", [&]() {
        QMessageBox::about(&window, "YJ-CAD",
            "YJ-CAD v0.1.0\n\n"
            "도무송 목형 전용 2D CAD 프로그램\n"
            "유진레이저목형\n\n"
            "C++ / Qt6 / OpenGL / Boost.Geometry / Clipper2");
    });

    window.show();
    return app.exec();
}
