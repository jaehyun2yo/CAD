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
#include <QToolBar>
#include <QTabWidget>
#include <QProgressDialog>

#include "core/document/document.h"
#include "core/fileio/dxf_reader.h"
#include "core/fileio/dxf_writer.h"
#include "core/fileio/dxf_load_worker.h"
#include "core/fileio/ai_eps_reader.h"
#include "core/logger.h"
#include "core/geometry/optimizer.h"
#include "ui/viewer_3d.h"
#include "ui/clipboard.h"
#include "ui/dreambox/box_dialog.h"
#include "ui/laser_panel.h"
#include "core/geometry/operations.h"
#include "ui/cad_canvas.h"
#include "ui/tool_manager.h"
#include "ui/domusong_panel.h"
#include "ui/properties_panel.h"
#include "ui/layer_panel.h"
#include "ui/command_line.h"
#include "ui/command_registry.h"

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

    // Initialize logging: %APPDATA%/YJ-CAD/logs/
    Logger::install();
    qInfo() << "YJ-CAD v0.1.0 starting";

    // Theme stylesheets
    const QString darkStyleSheet = R"(
        QMainWindow { background-color: #2b2b2b; }
        QMenuBar { background-color: #333333; color: #cccccc; }
        QMenuBar::item:selected { background-color: #4a90d9; }
        QMenu { background-color: #333333; color: #cccccc; }
        QMenu::item:selected { background-color: #4a90d9; }
        QStatusBar { background-color: #252525; color: #aaaaaa; }
        QDockWidget { color: #cccccc; }
        QDockWidget::title { background-color: #333333; padding: 4px; }
    )";

    const QString lightStyleSheet = R"(
        QMainWindow { background-color: #f0f0f0; }
        QMenuBar { background-color: #e8e8e8; color: #333333; }
        QMenuBar::item:selected { background-color: #4a90d9; color: white; }
        QMenu { background-color: #ffffff; color: #333333; }
        QMenu::item:selected { background-color: #4a90d9; color: white; }
        QStatusBar { background-color: #e0e0e0; color: #555555; }
        QDockWidget { color: #333333; }
        QDockWidget::title { background-color: #e0e0e0; padding: 4px; }
    )";

    // Default: Dark theme
    bool isDark = true;
    app.setStyleSheet(darkStyleSheet);

    Document doc;
    QMainWindow window;
    window.setWindowTitle("YJ-CAD v0.1.0 — 도무송 목형 전용 CAD");
    window.resize(1400, 900);

    // ── OpenGL Canvas (center) ──
    auto* canvas = new CadCanvas(doc);
    window.setCentralWidget(canvas);

    // ── 도무송 Panel (Right dock) ──
    auto* domusongPanel = new DomusongPanel(doc, canvas);
    auto* dockDomusong = new QDockWidget("도무송", &window);
    dockDomusong->setWidget(domusongPanel);
    dockDomusong->setMinimumWidth(260);
    dockDomusong->setStyleSheet("QDockWidget { color: #cccccc; }");
    window.addDockWidget(Qt::RightDockWidgetArea, dockDomusong);

    // ── Properties Panel (Right dock, tabbed) ──
    auto* propsPanel = new PropertiesPanel(doc, canvas);
    auto* dockProps = new QDockWidget("속성", &window);
    dockProps->setWidget(propsPanel);
    dockProps->setMinimumWidth(260);
    dockProps->setStyleSheet("QDockWidget { color: #cccccc; }");
    window.addDockWidget(Qt::RightDockWidgetArea, dockProps);
    window.tabifyDockWidget(dockDomusong, dockProps);

    // ── Layer Panel (Right dock, tabbed) ──
    auto* layerPanel = new LayerPanel(doc, canvas);
    auto* dockLayers = new QDockWidget("레이어", &window);
    dockLayers->setWidget(layerPanel);
    dockLayers->setMinimumWidth(260);
    dockLayers->setStyleSheet("QDockWidget { color: #cccccc; }");
    window.addDockWidget(Qt::RightDockWidgetArea, dockLayers);
    window.tabifyDockWidget(dockProps, dockLayers);

    // Default to 도무송 tab
    dockDomusong->raise();

    // (pipeline signal connected after helper lambdas below)

    // ── Status bar ──
    auto* coordLabel       = new QLabel("좌표: (0.00, 0.00)");
    auto* layerLabel       = new QLabel("레이어: 0");
    auto* snapLabel        = new QLabel("SNAP:ON");
    auto* orthoLabel       = new QLabel("ORTHO:OFF");
    auto* entityCountLabel = new QLabel("엔티티: 0");

    const QString statusLabelStyle = "color: #aaaaaa; padding: 0 8px;";
    coordLabel->setStyleSheet(statusLabelStyle);
    layerLabel->setStyleSheet(statusLabelStyle);
    snapLabel->setStyleSheet("color: #55aaff; padding: 0 8px; font-weight: bold;");
    orthoLabel->setStyleSheet("color: #888888; padding: 0 8px;");
    entityCountLabel->setStyleSheet(statusLabelStyle);

    window.statusBar()->addWidget(coordLabel);
    window.statusBar()->addWidget(layerLabel);
    window.statusBar()->addWidget(snapLabel);
    window.statusBar()->addWidget(orthoLabel);
    window.statusBar()->addPermanentWidget(entityCountLabel);

    QObject::connect(canvas, &CadCanvas::cursorWorldPos,
                     [coordLabel](double x, double y) {
        coordLabel->setText(QString("좌표: (%1, %2)")
                            .arg(x, 0, 'f', 2).arg(y, 0, 'f', 2));
    });

    // Snap/Ortho 상태 라벨 업데이트
    QObject::connect(canvas, &CadCanvas::snapEnabledChanged,
                     [snapLabel](bool enabled) {
        snapLabel->setText(enabled ? "SNAP:ON" : "SNAP:OFF");
        snapLabel->setStyleSheet(enabled
            ? "color: #55aaff; padding: 0 8px; font-weight: bold;"
            : "color: #888888; padding: 0 8px;");
    });
    QObject::connect(canvas, &CadCanvas::orthoModeChanged,
                     [orthoLabel](bool enabled) {
        orthoLabel->setText(enabled ? "ORTHO:ON" : "ORTHO:OFF");
        orthoLabel->setStyleSheet(enabled
            ? "color: #ffaa00; padding: 0 8px; font-weight: bold;"
            : "color: #888888; padding: 0 8px;");
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

    // Connect selection changes to properties panel
    QObject::connect(canvas, &CadCanvas::entityClicked, [propsPanel](EntityId) {
        propsPanel->onSelectionChanged();
    });

    // ── File menu ──
    auto* fileMenu = window.menuBar()->addMenu("파일(&F)");

    fileMenu->addAction("새 문서(&N)", [&]() {
        if (doc.isModified()) {
            auto reply = QMessageBox::question(&window, "새 문서",
                "저장되지 않은 변경이 있습니다. 계속하시겠습니까?",
                QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) return;
        }
        doc.clear();
        canvas->refreshEntities();
        updateTitle();
        updateEntityCount();
        layerPanel->refreshLayers();
        window.statusBar()->showMessage("새 문서", 3000);
    }, QKeySequence("Ctrl+N"));

    fileMenu->addAction("열기(&O)", [&]() {
        QString path = QFileDialog::getOpenFileName(&window, "DXF 파일 열기",
            QString(), "DXF 파일 (*.dxf);;모든 파일 (*)");
        if (path.isEmpty()) return;

        qInfo() << "Loading DXF:" << path;

        // Show progress dialog
        auto* progressDlg = new QProgressDialog("DXF 파일 로드 중...", "취소", 0, 100, &window);
        progressDlg->setWindowModality(Qt::WindowModal);
        progressDlg->setMinimumDuration(500); // show only if takes >500ms
        progressDlg->setValue(0);

        auto* task = new DxfLoadTask(doc, path, &window);

        QObject::connect(task, &DxfLoadTask::progress,
            [progressDlg](int percent, const QString& msg) {
                progressDlg->setValue(percent);
                progressDlg->setLabelText(msg);
            });

        QObject::connect(task, &DxfLoadTask::finished,
            [&, path, progressDlg, task](bool success, const QString& msg,
                                          int entityCount, int layerCount) {
                progressDlg->close();
                progressDlg->deleteLater();

                if (!success) {
                    QMessageBox::critical(&window, "오류", "DXF 로드 실패: " + msg);
                    qWarning() << "DXF load failed:" << msg;
                } else {
                    canvas->refreshEntities();
                    canvas->zoomToFit();
                    updateTitle();
                    updateEntityCount();
                    layerPanel->refreshLayers();

                    window.statusBar()->showMessage(
                        QString("%1 로드 완료 — %2개 엔티티, %3개 레이어")
                        .arg(path).arg(entityCount).arg(layerCount), 5000);
                    qInfo() << "DXF loaded:" << entityCount << "entities," << layerCount << "layers";
                }
                task->deleteLater();
            });

        task->start();
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

    fileMenu->addAction("AI/EPS 가져오기(&I)", [&]() {
        QString path = QFileDialog::getOpenFileName(&window, "AI/EPS 파일 가져오기",
            QString(), "AI/EPS 파일 (*.ai *.eps);;모든 파일 (*)");
        if (path.isEmpty()) return;

        qInfo() << "Importing AI/EPS:" << path;
        AiEpsReader reader(doc);
        auto result = reader.load(path.toStdString());

        if (!result.success) {
            QMessageBox::critical(&window, "오류",
                QString::fromStdString("AI/EPS 가져오기 실패: " + result.errorMessage));
            return;
        }

        canvas->refreshEntities();
        canvas->zoomToFit();
        updateTitle();
        updateEntityCount();
        layerPanel->refreshLayers();

        window.statusBar()->showMessage(
            QString("AI/EPS 가져오기 완료 — %1개 엔티티, %2개 경로")
            .arg(result.entityCount).arg(result.pathCount), 5000);
        qInfo() << "AI/EPS imported:" << result.entityCount << "entities";
    }, QKeySequence("Ctrl+I"));

    fileMenu->addSeparator();

    // Print — requires Qt6::PrintSupport (TODO: add when available)
    fileMenu->addAction("인쇄(&P)", [&]() {
        window.statusBar()->showMessage("인쇄: Qt PrintSupport 모듈 필요 (향후 추가)", 5000);
    }, QKeySequence("Ctrl+P"));

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

    editMenu->addSeparator();

    // Clipboard
    static CadClipboard clipboard;
    editMenu->addAction("복사(&C)", [&]() {
        clipboard.copy(doc);
        window.statusBar()->showMessage(
            QString("클립보드 복사: %1개 엔티티").arg(clipboard.count()), 3000);
    }, QKeySequence("Ctrl+C"));

    editMenu->addAction("붙여넣기(&V)", [&]() {
        if (!clipboard.hasData()) return;
        auto bounds = doc.bounds();
        Point2D center{(bounds.min.x + bounds.max.x) / 2.0,
                        (bounds.min.y + bounds.max.y) / 2.0};
        clipboard.paste(doc, center);
        canvas->refreshEntities();
        updateEntityCount();
        window.statusBar()->showMessage("붙여넣기 완료", 3000);
    }, QKeySequence("Ctrl+V"));

    // ── View menu ──
    auto* viewMenu = window.menuBar()->addMenu("보기(&V)");
    viewMenu->addAction("줌 투 핏(&F)", [canvas]() {
        canvas->zoomToFit();
    }, QKeySequence("Home"));

    viewMenu->addSeparator();
    viewMenu->addAction("스냅 토글(&N)  [F3]", [canvas, &window]() {
        canvas->toggleSnap();
        window.statusBar()->showMessage(
            canvas->isSnapEnabled() ? "스냅 ON" : "스냅 OFF", 2000);
    }, QKeySequence("F3"));

    viewMenu->addAction("Ortho 모드(&H)  [F8]", [canvas, &window]() {
        canvas->toggleOrthoMode();
        window.statusBar()->showMessage(
            canvas->isOrthoMode() ? "직교(Ortho) ON" : "직교(Ortho) OFF", 2000);
    }, QKeySequence("F8"));

    // ── 3D Viewer (bottom dock) ──
    auto* viewer3dPanel = new Viewer3DPanel(doc);
    auto* dock3d = new QDockWidget("3D 뷰어", &window);
    dock3d->setWidget(viewer3dPanel);
    dock3d->setMinimumHeight(200);
    dock3d->setStyleSheet("QDockWidget { color: #cccccc; }");
    window.addDockWidget(Qt::BottomDockWidgetArea, dock3d);
    dock3d->hide(); // hidden by default

    viewMenu->addSeparator();
    viewMenu->addAction("3D 뷰어(&3)", [dock3d, viewer3dPanel]() {
        dock3d->setVisible(!dock3d->isVisible());
        if (dock3d->isVisible()) {
            viewer3dPanel->viewer()->rebuildFromDocument();
        }
    }, QKeySequence("F11"));

    viewMenu->addSeparator();
    viewMenu->addAction("테마 전환 (다크/라이트)(&T)", [&]() {
        isDark = !isDark;
        app.setStyleSheet(isDark ? darkStyleSheet : lightStyleSheet);
        canvas->setDarkTheme(isDark);
        window.statusBar()->showMessage(
            isDark ? "다크 테마 적용" : "라이트 테마 적용", 3000);
        qInfo() << "Theme switched to" << (isDark ? "dark" : "light");
    }, QKeySequence("F12"));

    // ── Draw menu ──
    auto* drawMenu = window.menuBar()->addMenu("그리기(&D)");
    auto setTool = [&](ToolMode mode, const QString& name) {
        canvas->toolManager().setTool(mode);
        window.statusBar()->showMessage("도구: " + name, 3000);
    };

    drawMenu->addAction("선(&L)", [&]() { setTool(ToolMode::DrawLine, "선 그리기"); });
    drawMenu->addAction("원(&C)", [&]() { setTool(ToolMode::DrawCircle, "원 그리기"); });
    drawMenu->addAction("호(&A)", [&]() { setTool(ToolMode::DrawArc, "호 그리기"); });
    drawMenu->addAction("폴리라인(&P)", [&]() { setTool(ToolMode::DrawPolyline, "폴리라인"); });
    drawMenu->addSeparator();
    drawMenu->addAction("사각형(&R)", [&]() { setTool(ToolMode::DrawRect, "사각형"); });
    drawMenu->addAction("타원(&E)", [&]() { setTool(ToolMode::DrawEllipse, "타원"); });
    drawMenu->addAction("다각형(&G)", [&]() { setTool(ToolMode::DrawPolygon, "다각형"); });
    drawMenu->addAction("점(&T)", [&]() { setTool(ToolMode::DrawPoint, "점"); });
    drawMenu->addAction("텍스트(&X)", [&]() { setTool(ToolMode::DrawText, "텍스트"); });
    drawMenu->addAction("구성선(&N)", [&]() { setTool(ToolMode::DrawXLine, "구성선"); });

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

    modifyMenu->addAction("이동(&M)", [&]() { setTool(ToolMode::Move, "이동"); });
    modifyMenu->addAction("복사(&C)", [&]() { setTool(ToolMode::Copy, "복사"); });
    modifyMenu->addAction("회전(&R)", [&]() { setTool(ToolMode::Rotate, "회전"); });
    modifyMenu->addAction("미러(&I)", [&]() { setTool(ToolMode::Mirror, "미러"); });
    modifyMenu->addAction("스케일(&S)", [&]() { setTool(ToolMode::Scale, "스케일"); });
    modifyMenu->addAction("오프셋(&O)", [&]() { setTool(ToolMode::Offset, "오프셋"); });
    modifyMenu->addAction("트림(&T)", [&]() { setTool(ToolMode::Trim, "트림"); });
    modifyMenu->addSeparator();
    modifyMenu->addAction("배열(&A)", [&]() { setTool(ToolMode::Array, "배열"); });
    modifyMenu->addAction("원형 배열", [&]() { setTool(ToolMode::PolarArray, "원형 배열"); });
    modifyMenu->addAction("스트레치", [&]() { setTool(ToolMode::Stretch, "스트레치"); });
    modifyMenu->addAction("연장(&E)", [&]() { setTool(ToolMode::Extend, "연장"); });
    modifyMenu->addAction("브레이크(&B)", [&]() { setTool(ToolMode::Break, "브레이크"); });
    modifyMenu->addAction("분할(&V)", [&]() { setTool(ToolMode::Divide, "분할"); });
    modifyMenu->addAction("모깎기", [&]() { setTool(ToolMode::Chamfer, "모깎기"); });
    modifyMenu->addAction("필릿(&F)", [&]() { setTool(ToolMode::Fillet, "필릿"); });

    // ── Group menu ──
    auto* groupMenu = window.menuBar()->addMenu("그룹(&G)");
    groupMenu->addAction("결합(&J)", [&]() { setTool(ToolMode::Join, "결합"); });
    groupMenu->addAction("분해(&X)", [&]() { setTool(ToolMode::Explode, "분해"); });

    // ── Inquiry menu ──
    auto* inquiryMenu = window.menuBar()->addMenu("조회(&Q)");
    inquiryMenu->addAction("거리(&D)", [&]() { setTool(ToolMode::Distance, "거리 측정"); });
    inquiryMenu->addAction("면적(&A)", [&]() { setTool(ToolMode::AreaCalc, "면적 계산"); });
    inquiryMenu->addAction("치수(&I)", [&]() { setTool(ToolMode::Dim, "선형 치수"); });
    inquiryMenu->addAction("줌 윈도(&W)", [&]() { setTool(ToolMode::ZoomWindow, "줌 윈도"); });

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
    }, QKeySequence("Ctrl+F8"));

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

    // ── Optimize menu ──
    auto* optimizeMenu = window.menuBar()->addMenu("최적화(&O)");
    optimizeMenu->addAction("정리 (미세 엔티티 제거)", [&]() {
        Optimizer opt(doc);
        int removed = opt.clean(0.01);
        canvas->refreshEntities();
        updateEntityCount();
        window.statusBar()->showMessage(QString("정리 완료 — %1개 제거").arg(removed), 5000);
    });
    optimizeMenu->addAction("중복 제거", [&]() {
        Optimizer opt(doc);
        int removed = opt.removeOverlaps(0.01);
        canvas->refreshEntities();
        updateEntityCount();
        window.statusBar()->showMessage(QString("중복 제거 — %1개 제거").arg(removed), 5000);
    });
    optimizeMenu->addAction("스냅 결합", [&]() {
        Optimizer opt(doc);
        int joined = opt.snapJoin(0.1);
        canvas->refreshEntities();
        window.statusBar()->showMessage(QString("스냅 결합 — %1개 연결").arg(joined), 5000);
    });
    optimizeMenu->addSeparator();
    optimizeMenu->addAction("총 길이 계산", [&]() {
        Optimizer opt(doc);
        auto sel = doc.selection().selectedIds();
        std::vector<EntityId> ids(sel.begin(), sel.end());
        if (ids.empty()) ids = doc.entityIds();
        double total = opt.totalLength(ids);
        window.statusBar()->showMessage(
            QString("총 길이: %1 mm").arg(total, 0, 'f', 2), 10000);
    });
    optimizeMenu->addAction("분류별 길이", [&]() {
        Optimizer opt(doc);
        auto sel = doc.selection().selectedIds();
        std::vector<EntityId> ids(sel.begin(), sel.end());
        if (ids.empty()) ids = doc.entityIds();
        auto lengths = opt.lengthByClass(ids);
        QString msg;
        static const char* names[] = {"미분류", "오시", "칼", "구아이", "브릿지", "외곽"};
        for (auto& [cls, len] : lengths) {
            msg += QString("%1: %2mm  ").arg(names[static_cast<int>(cls)]).arg(len, 0, 'f', 1);
        }
        window.statusBar()->showMessage(msg, 10000);
    });

    // ── DreamBox menu ──
    auto* dreamboxMenu = window.menuBar()->addMenu("DreamBox(&B)");
    dreamboxMenu->addAction("박스 생성...", [&]() {
        BoxDialog dlg(doc, &window);
        if (dlg.exec() == QDialog::Accepted) {
            canvas->refreshEntities();
            canvas->zoomToFit();
            updateEntityCount();
            layerPanel->refreshLayers();
            window.statusBar()->showMessage("박스 생성 완료", 5000);
        }
    });

    // ── Laser panel (Right dock, tabbed) ──
    auto* laserPanel = new LaserPanel(doc, canvas);
    auto* dockLaser = new QDockWidget("레이저", &window);
    dockLaser->setWidget(laserPanel);
    dockLaser->setMinimumWidth(260);
    dockLaser->setStyleSheet("QDockWidget { color: #cccccc; }");
    window.addDockWidget(Qt::RightDockWidgetArea, dockLaser);
    window.tabifyDockWidget(dockLayers, dockLaser);

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

    // ── Left Icon Toolbar ──
    auto* toolbar = new QToolBar("도구 모음", &window);
    toolbar->setOrientation(Qt::Vertical);
    toolbar->setIconSize(QSize(24, 24));
    toolbar->setMovable(false);
    toolbar->setStyleSheet(
        "QToolBar { background: #333333; border: none; spacing: 2px; padding: 4px; }"
        "QToolButton { background: transparent; color: #cccccc; border: 1px solid transparent; "
        "border-radius: 3px; padding: 4px; min-width: 28px; min-height: 28px; font-size: 11px; }"
        "QToolButton:hover { background: #4a4a4a; border: 1px solid #666; }"
        "QToolButton:checked { background: #4a90d9; border: 1px solid #5ba0e9; }");
    window.addToolBar(Qt::LeftToolBarArea, toolbar);

    // Helper to add toolbar action with text label (icons can be added later)
    auto addTbAction = [&](const QString& label, ToolMode mode, const QString& name) {
        auto* action = toolbar->addAction(label);
        action->setToolTip(name);
        action->setCheckable(true);
        QObject::connect(action, &QAction::triggered, [&, mode, name]() {
            setTool(mode, name);
        });
        return action;
    };

    toolbar->addAction("▶")->setToolTip("선택");
    QObject::connect(toolbar->actions().last(), &QAction::triggered, [&]() {
        canvas->toolManager().cancelToSelect();
        window.statusBar()->showMessage("도구: 선택", 2000);
    });
    toolbar->addSeparator();

    addTbAction("╱", ToolMode::DrawLine, "선 그리기");
    addTbAction("◯", ToolMode::DrawCircle, "원 그리기");
    addTbAction("◠", ToolMode::DrawArc, "호 그리기");
    addTbAction("⬡", ToolMode::DrawPolyline, "폴리라인");
    toolbar->addSeparator();

    addTbAction("✥", ToolMode::Move, "이동");
    addTbAction("⧉", ToolMode::Copy, "복사");
    addTbAction("↻", ToolMode::Rotate, "회전");
    addTbAction("⇿", ToolMode::Mirror, "미러");
    addTbAction("⊡", ToolMode::Scale, "스케일");
    addTbAction("⟧", ToolMode::Offset, "오프셋");
    addTbAction("✂", ToolMode::Trim, "트림");
    addTbAction("✕", ToolMode::Delete, "삭제");

    // Refresh layer panel after file open
    QObject::connect(canvas, &CadCanvas::entityClicked, [layerPanel](EntityId) {
        layerPanel->refreshLayers();
    });

    // ── Command Line Widget (bottom dock) ──────────────────
    auto* cmdLine = new CommandLineWidget(&window);
    auto* dockCmd = new QDockWidget("명령행", &window);
    dockCmd->setWidget(cmdLine);
    dockCmd->setFeatures(QDockWidget::NoDockWidgetFeatures); // fixed, no float/close
    dockCmd->setTitleBarWidget(new QWidget()); // hide title bar
    dockCmd->setFixedHeight(56);
    dockCmd->setStyleSheet("QDockWidget { padding: 0; margin: 0; }");
    window.addDockWidget(Qt::BottomDockWidgetArea, dockCmd);

    // ── Command Registry ───────────────────────────────────
    CommandRegistry cmdReg;

    // Helper: register a tool command
    auto regTool = [&](const QString& alias, const QString& name, ToolMode mode) {
        cmdReg.registerCommand(alias, name, [&canvas = *canvas, &window, mode, name]() {
            canvas.toolManager().setTool(mode);
            window.statusBar()->showMessage("도구: " + name, 3000);
        });
    };

    // ── STANDARD ──────────────────────────────────────────
    cmdReg.registerCommand("NEW",  "New",     [&]() {
        window.statusBar()->showMessage("미구현 명령: NEW", 3000); });
    cmdReg.registerCommand("OPEN", "Open",    [&]() {
        window.statusBar()->showMessage("미구현 명령: OPEN", 3000); });
    cmdReg.registerCommand("SAVE", "Save",    [&]() {
        // Reuse the save logic via menu shortcut simulation
        window.statusBar()->showMessage("미구현 명령: SAVE (단축키 Ctrl+S 사용)", 3000); });
    cmdReg.registerCommand("ESC",  "Cancel",  [&]() {
        canvas->toolManager().cancelToSelect();
        window.statusBar()->showMessage("도구: 선택", 2000); });
    cmdReg.registerCommand("DXI",  "DxfOpen", [&]() {
        window.statusBar()->showMessage("미구현 명령: DXI (단축키 Ctrl+O 사용)", 3000); });
    cmdReg.registerCommand("DXO",  "DxfSave", [&]() {
        window.statusBar()->showMessage("미구현 명령: DXO (단축키 Ctrl+S 사용)", 3000); });
    cmdReg.registerCommand("U",    "Undo",    [&]() {
        if (doc.history().undo()) { canvas->refreshEntities(); updateTitle(); } });
    cmdReg.registerCommand("RD",   "Redo",    [&]() {
        if (doc.history().redo()) { canvas->refreshEntities(); updateTitle(); } });

    // ── DRAW ──────────────────────────────────────────────
    regTool("L",      "선 그리기",        ToolMode::DrawLine);
    regTool("XL",     "구성선",          ToolMode::DrawXLine);
    regTool("A",      "호 그리기",        ToolMode::DrawArc);
    regTool("C",      "원 그리기",        ToolMode::DrawCircle);
    regTool("PL",     "폴리라인",         ToolMode::DrawPolyline);
    regTool("BE",     "베지어",           ToolMode::DrawPolyline); // fallback

    // Unimplemented draw commands
    auto regUnimpl = [&](const QString& alias, const QString& name) {
        cmdReg.registerCommand(alias, name, [&window, name]() {
            window.statusBar()->showMessage("미구현 명령: " + name, 3000); });
    };
    regTool("EL",     "타원",            ToolMode::DrawEllipse);
    regTool("RERERE", "사각형",          ToolMode::DrawRect);
    regTool("POL",    "다각형",          ToolMode::DrawPolygon);
    regUnimpl("SR",     "Star");
    regTool("PT",     "점",              ToolMode::DrawPoint);
    regTool("T",      "텍스트",          ToolMode::DrawText);
    regUnimpl("OL",     "Outline");

    // ── ZOOM ──────────────────────────────────────────────
    cmdReg.registerCommand("Z",   "Zoom",    [&]() { canvas->zoomToFit(); });
    cmdReg.registerCommand("ZW",  "ZoomWin", [&]() { canvas->zoomToFit(); });
    cmdReg.registerCommand("ZA",  "ZoomAll", [&]() { canvas->zoomToFit(); });
    cmdReg.registerCommand("ZE",  "ZoomExt", [&]() { canvas->zoomToFit(); });
    cmdReg.registerCommand("ZI",  "ZoomIn",  [&]() {
        window.statusBar()->showMessage("미구현 명령: ZoomIn", 3000); });
    cmdReg.registerCommand("ZO",  "ZoomOut", [&]() {
        window.statusBar()->showMessage("미구현 명령: ZoomOut", 3000); });
    cmdReg.registerCommand("ZP",  "ZoomPre", [&]() {
        window.statusBar()->showMessage("미구현 명령: ZoomPre", 3000); });
    cmdReg.registerCommand("P",   "Pan",     [&]() {
        window.statusBar()->showMessage("미구현 명령: Pan", 3000); });
    cmdReg.registerCommand("RE",  "Redraw",  [&]() { canvas->refreshEntities(); });

    // ── INQUIRY ──────────────────────────────────────────
    regTool("D",   "거리 측정",  ToolMode::Distance);
    regUnimpl("ID",  "ID");
    regUnimpl("LI",  "List");
    regTool("AR",  "면적 계산",  ToolMode::AreaCalc);

    // ── CHANGE (MODIFY) ──────────────────────────────────
    cmdReg.registerCommand("E",   "Erase",   [&]() {
        auto selected = doc.selection().selectedIds();
        for (EntityId id : selected) {
            auto cmd = std::make_unique<RemoveEntityCommand>(doc, id);
            doc.history().execute(std::move(cmd));
        }
        doc.selection().clear();
        canvas->refreshEntities();
        updateEntityCount();
        updateTitle();
    });
    cmdReg.registerCommand("DEL", "Erase",   [&]() { cmdReg.execute("E"); });

    regTool("CO", "복사",   ToolMode::Copy);
    regTool("M",  "이동",   ToolMode::Move);
    regTool("RO", "회전",   ToolMode::Rotate);
    regTool("MI", "미러",   ToolMode::Mirror);
    regTool("SC", "스케일", ToolMode::Scale);
    regTool("O",  "오프셋", ToolMode::Offset);
    regTool("TR", "트림",   ToolMode::Trim);

    regTool("AP",  "원형 배열",  ToolMode::PolarArray);
    regTool("ST",  "스트레치",  ToolMode::Stretch);
    regTool("ET",  "연장",      ToolMode::Extend);
    regTool("BR",  "브레이크",  ToolMode::Break);
    regTool("DIV", "분할",      ToolMode::Divide);
    regTool("CHA", "모깎기",    ToolMode::Chamfer);
    regTool("F",   "필릿",      ToolMode::Fillet);
    regUnimpl("FO",  "Fillet0");

    // ── TOOL (도무송 전용) ────────────────────────────────
    regUnimpl("IP",  "InsPoint");
    regUnimpl("DP",  "DelPoint");
    regUnimpl("DDB", "BridgeConfig");
    regUnimpl("TL",  "TotalLen");

    // ── GROUP ────────────────────────────────────────────
    regTool("JO",  "결합",   ToolMode::Join);
    regUnimpl("SJO", "StepJoin");
    regTool("X",   "분해",   ToolMode::Explode);
    regUnimpl("AL",  "Alignment");

    // ── DIM ──────────────────────────────────────────────
    regTool("DL",  "치수선",      ToolMode::Dim);
    regTool("DA",  "각도치수",    ToolMode::Dim);
    regTool("DR",  "반지름치수",  ToolMode::Dim);
    regTool("DD",  "지름치수",    ToolMode::Dim);
    regUnimpl("DS",  "DimSingle");

    // ── LASER ────────────────────────────────────────────
    regUnimpl("DDLAS", "LaserConfig");
    regUnimpl("AUTO",  "AutoPath");
    regUnimpl("NCS",   "NcSave");
    regUnimpl("NCC",   "NcSend");
    regUnimpl("SM",    "Simulation");

    // ── OSNAP ────────────────────────────────────────────
    regUnimpl("END", "OsnapEnd");
    regUnimpl("NEA", "OsnapNearest");
    regUnimpl("MID", "OsnapMid");
    regUnimpl("INT", "OsnapIntersect");
    regUnimpl("PER", "OsnapPerpendicular");
    regUnimpl("CEN", "OsnapCenter");
    regUnimpl("TAN", "OsnapTangent");
    regUnimpl("QUA", "OsnapQuadrant");
    regUnimpl("CAN", "OsnapCancel");
    regUnimpl("DDOS","OsnapSettings");

    // ── ETC ──────────────────────────────────────────────
    regUnimpl("LA",   "LayerConfig");
    regUnimpl("DDST", "StyleConfig");

    // ── Connect CommandLineWidget to registry ─────────────
    QObject::connect(cmdLine, &CommandLineWidget::commandEntered,
                     [&](const QString& cmd) {
        if (cmd == "ESC")
            return; // already handled by ESC key handler in widget
        const bool found = cmdReg.execute(cmd);
        if (!found) {
            cmdLine->showMessage(QString("알 수 없는 명령: %1").arg(cmd));
            window.statusBar()->showMessage(
                QString("알 수 없는 명령: %1").arg(cmd), 3000);
        } else {
            const QString name = cmdReg.nameOf(cmd);
            cmdLine->showMessage(
                name.isEmpty() ? cmd : QString("%1 실행됨").arg(name));
        }
    });

    // Coordinate signals: forward to status bar for now
    // (tools can connect to these when they need point input)
    QObject::connect(cmdLine, &CommandLineWidget::coordinateEntered,
                     [&](double x, double y) {
        window.statusBar()->showMessage(
            QString("좌표 입력: (%1, %2)").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2), 3000);
    });
    QObject::connect(cmdLine, &CommandLineWidget::relativeCoordinateEntered,
                     [&](double dx, double dy) {
        window.statusBar()->showMessage(
            QString("상대좌표 입력: @(%1, %2)").arg(dx, 0, 'f', 2).arg(dy, 0, 'f', 2), 3000);
    });

    window.show();
    return app.exec();
}
