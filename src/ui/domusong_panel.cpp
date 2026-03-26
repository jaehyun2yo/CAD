#include "domusong_panel.h"
#include "cad_canvas.h"
#include "core/geometry/operations.h"
#include "core/fileio/dxf_writer.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>

namespace yjcad {

DomusongPanel::DomusongPanel(Document& doc, CadCanvas* canvas, QWidget* parent)
    : QWidget(parent), m_doc(doc), m_canvas(canvas) {

    m_steps = {
        {"정리", "불필요한 엔티티 제거", false},
        {"분류", "오시/칼선 분류", true},
        {"브릿지", "칼선에 브릿지 삽입", false},
        {"점빼기", "호→선분 변환", false},
        {"정타/후타", "방향 판단 + 뒤집기", true},
        {"씨아기칼", "인쇄지 분할선 생성", false},
        {"외곽선", "절수 맞춤 외곽선", false},
        {"구아이선", "가이드선 삽입", false},
        {"텍스트", "정보 텍스트 작성", false},
        {"저장", "파일명 규칙 저장", false},
    };

    setupUI();
}

void DomusongPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // ── Config group ──
    m_configGroup = new QGroupBox("작업 정보");
    auto* configLayout = new QVBoxLayout(m_configGroup);

    auto addRow = [&](const QString& label, QWidget* widget) {
        auto* row = new QHBoxLayout();
        auto* lbl = new QLabel(label);
        lbl->setFixedWidth(60);
        row->addWidget(lbl);
        row->addWidget(widget);
        configLayout->addLayout(row);
    };

    m_companyEdit = new QLineEdit();
    m_companyEdit->setPlaceholderText("업체명");
    addRow("업체", m_companyEdit);

    m_drawingNameEdit = new QLineEdit();
    m_drawingNameEdit->setPlaceholderText("도면명");
    addRow("도면", m_drawingNameEdit);

    m_jeolsuCombo = new QComboBox();
    m_jeolsuCombo->addItems({"2절", "4절", "8절", "국2절", "국4절", "국8절"});
    addRow("절수", m_jeolsuCombo);

    m_printSizeCombo = new QComboBox();
    m_printSizeCombo->addItems({"A3", "A4", "B4", "B3", "A2", "국전"});
    addRow("인쇄지", m_printSizeCombo);

    m_orientationCombo = new QComboBox();
    m_orientationCombo->addItems({"정타", "후타"});
    addRow("방향", m_orientationCombo);

    layout->addWidget(m_configGroup);

    // ── Run buttons ──
    m_runAllBtn = new QPushButton("▶ 전체 실행 (F5)");
    m_runAllBtn->setStyleSheet(
        "QPushButton { background-color: #4a90d9; color: white; padding: 8px; "
        "font-weight: bold; border-radius: 4px; }"
        "QPushButton:hover { background-color: #5aa0e9; }");
    connect(m_runAllBtn, &QPushButton::clicked, this, &DomusongPanel::onRunPipeline);
    layout->addWidget(m_runAllBtn);

    auto* btnRow = new QHBoxLayout();
    m_runStepBtn = new QPushButton("▶ 단계 실행");
    m_skipBtn = new QPushButton("⏭ 건너뜀");
    m_runStepBtn->setStyleSheet("padding: 4px;");
    m_skipBtn->setStyleSheet("padding: 4px;");
    connect(m_runStepBtn, &QPushButton::clicked, [this]() { onRunSingleStep(m_currentStep); });
    connect(m_skipBtn, &QPushButton::clicked, this, &DomusongPanel::onSkipStep);
    btnRow->addWidget(m_runStepBtn);
    btnRow->addWidget(m_skipBtn);
    layout->addLayout(btnRow);

    // ── Progress ──
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 10);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("%v/10 단계");
    layout->addWidget(m_progressBar);

    // ── Step list ──
    m_stepList = new QListWidget();
    updateStepList();
    layout->addWidget(m_stepList, 1);

    // ── Status ──
    m_statusLabel = new QLabel("준비");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #aaaaaa; padding: 4px;");
    layout->addWidget(m_statusLabel);

    // Style
    setStyleSheet(
        "QGroupBox { color: #cccccc; border: 1px solid #555; border-radius: 4px; "
        "margin-top: 8px; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; }"
        "QLineEdit, QComboBox { background-color: #3a3a3a; color: #cccccc; "
        "border: 1px solid #555; border-radius: 3px; padding: 3px; }"
        "QLabel { color: #aaaaaa; }"
        "QListWidget { background-color: #2a2a2a; color: #cccccc; border: 1px solid #444; }"
        "QProgressBar { background-color: #3a3a3a; border: 1px solid #555; "
        "border-radius: 3px; text-align: center; color: #cccccc; }"
        "QProgressBar::chunk { background-color: #4a90d9; border-radius: 2px; }");
}

void DomusongPanel::updateStepList() {
    m_stepList->clear();
    for (int i = 0; i < static_cast<int>(m_steps.size()); ++i) {
        QString icon = (i < m_currentStep) ? "✅" : (i == m_currentStep ? "▶" : "⬜");
        QString confirm = m_steps[i].requiresConfirm ? " [확인]" : "";
        m_stepList->addItem(QString("%1 %2. %3 — %4%5")
            .arg(icon)
            .arg(i + 1)
            .arg(m_steps[i].name)
            .arg(m_steps[i].description)
            .arg(confirm));
    }
    if (m_currentStep < static_cast<int>(m_steps.size())) {
        m_stepList->setCurrentRow(m_currentStep);
    }
}

void DomusongPanel::setStepStatus(int index, const QString& status, const QString& icon) {
    if (index < 0 || index >= m_stepList->count()) return;
    auto* item = m_stepList->item(index);
    item->setText(QString("%1 %2. %3 — %4")
        .arg(icon).arg(index + 1).arg(m_steps[index].name).arg(status));
}

void DomusongPanel::onRunPipeline() {
    if (m_doc.entityCount() == 0) {
        QMessageBox::information(this, "도무송", "먼저 DXF 파일을 열어주세요.");
        return;
    }

    m_currentStep = 0;
    m_progressBar->setValue(0);
    updateStepList();
    m_statusLabel->setText("파이프라인 실행 중...");

    // Run all steps sequentially
    while (m_currentStep < static_cast<int>(m_steps.size())) {
        // 반자동 단계: 확인 요청
        if (m_steps[m_currentStep].requiresConfirm) {
            auto reply = QMessageBox::question(this, "확인 필요",
                QString("%1 단계를 실행하시겠습니까?\n%2")
                .arg(m_steps[m_currentStep].name)
                .arg(m_steps[m_currentStep].description),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort);

            if (reply == QMessageBox::Abort) {
                m_statusLabel->setText("파이프라인 중단됨");
                updateStepList();
                return;
            }
            if (reply == QMessageBox::No) {
                onSkipStep();
                continue;
            }
        }

        onRunSingleStep(m_currentStep);
    }

    m_statusLabel->setText("✅ 파이프라인 완료!");
    emit pipelineCompleted();
}

void DomusongPanel::onRunSingleStep(int stepIndex) {
    if (stepIndex < 0 || stepIndex >= static_cast<int>(m_steps.size())) return;

    QString stepName = m_steps[stepIndex].name;
    setStepStatus(stepIndex, "실행 중...", "⏳");

    bool success = true;
    QString message;

    // Execute step
    if (stepName == "정리") {
        // Remove zero-length lines
        int removed = 0;
        auto ids = m_doc.entityIds();
        for (auto id : ids) {
            auto* e = m_doc.entity(id);
            if (!e) continue;
            if (e->type() == EntityType::Line) {
                auto* line = static_cast<LineEntity*>(e);
                if (line->length() < 0.001) {
                    m_doc.removeEntity(id);
                    ++removed;
                }
            }
        }
        message = QString("%1개 엔티티 정리됨").arg(removed);

    } else if (stepName == "분류") {
        // Simple classification by color
        int classified = 0;
        for (auto id : m_doc.entityIds()) {
            auto* e = m_doc.entity(id);
            if (!e || e->classification() != LineClassification::Unknown) continue;
            if (e->color() == 1) { e->setClassification(LineClassification::Oshi); ++classified; }
            else { e->setClassification(LineClassification::Knife); ++classified; }
        }
        message = QString("%1개 엔티티 분류됨").arg(classified);

    } else if (stepName == "브릿지") {
        int created = 0;
        auto ids = m_doc.entityIds();
        for (auto id : ids) {
            auto* e = m_doc.entity(id);
            if (!e || e->type() != EntityType::Line) continue;
            if (e->classification() != LineClassification::Knife) continue;
            auto* line = static_cast<LineEntity*>(e);
            if (line->length() < 25.0) continue; // skip short lines

            auto segs = GeometryOps::insertBridges(*line, 3.0, 20.0, m_doc.nextId());
            if (segs.size() > 1) {
                m_doc.removeEntity(id);
                for (auto& seg : segs) {
                    seg->setClassification(LineClassification::Knife);
                    m_doc.addEntity(std::move(seg));
                }
                created += static_cast<int>(segs.size());
            }
        }
        message = QString("브릿지 삽입: %1개 세그먼트").arg(created);

    } else if (stepName == "점빼기") {
        int converted = 0;
        auto ids = m_doc.entityIds();
        for (auto id : ids) {
            auto* e = m_doc.entity(id);
            if (!e || e->type() != EntityType::Arc) continue;
            auto* arc = static_cast<ArcEntity*>(e);
            auto poly = GeometryOps::arcToPolyline(*arc, 0.1, m_doc.nextId());
            if (poly) {
                m_doc.removeEntity(id);
                m_doc.addEntity(std::move(poly));
                ++converted;
            }
        }
        message = QString("%1개 호 → 선분 변환").arg(converted);

    } else if (stepName == "정타/후타") {
        QString orient = m_orientationCombo->currentText();
        if (orient == "정타") {
            auto bounds = m_doc.bounds();
            double cx = (bounds.min.x + bounds.max.x) / 2.0;
            for (auto id : m_doc.entityIds()) {
                if (auto* e = m_doc.entity(id))
                    e->mirror(Point2D{cx, 0}, Point2D{cx, 1});
            }
            message = "정타: 도면 뒤집기(미러) 완료";
        } else {
            message = "후타: 도면 그대로 사용";
        }

    } else if (stepName == "씨아기칼") {
        auto bounds = m_doc.bounds();
        double printWidth = 297.0; // A3 default
        QString ps = m_printSizeCombo->currentText();
        if (ps == "A4") printWidth = 210;
        else if (ps == "B4") printWidth = 257;
        else if (ps == "B3") printWidth = 364;

        double drawingWidth = bounds.max.x - bounds.min.x;
        int count = 0;
        if (drawingWidth > printWidth) {
            double x = bounds.min.x + printWidth;
            while (x < bounds.max.x) {
                auto id = m_doc.nextId();
                auto cutLine = std::make_unique<LineEntity>(id,
                    Point2D{x, bounds.min.y - 5}, Point2D{x, bounds.max.y + 5});
                cutLine->setClassification(LineClassification::Knife);
                cutLine->setLayerName("cutline");
                m_doc.layers().addLayer("cutline", 5);
                m_doc.addEntity(std::move(cutLine));
                x += printWidth;
                ++count;
            }
        }
        message = QString("씨아기칼 %1개 생성 (인쇄지: %2)").arg(count).arg(ps);

    } else if (stepName == "외곽선") {
        auto bounds = m_doc.bounds();
        double margin = 15.0;
        auto id = m_doc.nextId();
        std::vector<Point2D> verts = {
            {bounds.min.x - margin, bounds.min.y - margin},
            {bounds.max.x + margin, bounds.min.y - margin},
            {bounds.max.x + margin, bounds.max.y + margin},
            {bounds.min.x - margin, bounds.max.y + margin}
        };
        auto outline = std::make_unique<PolylineEntity>(id, verts, true);
        outline->setClassification(LineClassification::Outline);
        outline->setLayerName("outline");
        m_doc.layers().addLayer("outline", 4);
        m_doc.addEntity(std::move(outline));
        message = "외곽선 생성 완료";

    } else if (stepName == "구아이선") {
        auto bounds = m_doc.bounds();
        double y = bounds.min.y - 10.0;
        auto id = m_doc.nextId();
        auto guide = std::make_unique<LineEntity>(id,
            Point2D{bounds.min.x - 15, y}, Point2D{bounds.max.x + 15, y});
        guide->setClassification(LineClassification::Guide);
        guide->setLayerName("guide");
        m_doc.layers().addLayer("guide", 2);
        m_doc.addEntity(std::move(guide));
        message = "구아이선 삽입 (하단 10mm)";

    } else if (stepName == "텍스트") {
        auto bounds = m_doc.bounds();
        QString date = QDateTime::currentDateTime().toString("MM/dd");
        QString text = QString("%1 %2 %3 %4")
            .arg(date)
            .arg(m_companyEdit->text())
            .arg(m_drawingNameEdit->text())
            .arg(m_printSizeCombo->currentText());
        auto id = m_doc.nextId();
        auto textEnt = std::make_unique<TextEntity>(id,
            Point2D{bounds.min.x - 15, bounds.max.y + 20},
            text.toStdString(), 5.0);
        m_doc.addEntity(std::move(textEnt));
        message = "정보 텍스트: " + text;

    } else if (stepName == "저장") {
        QString date = QDateTime::currentDateTime().toString("MMdd");
        QString filename = QString("%1-1 %2 %3 %4.DXF")
            .arg(date)
            .arg(m_companyEdit->text())
            .arg(m_drawingNameEdit->text())
            .arg(m_jeolsuCombo->currentText());

        QString path = QFileDialog::getSaveFileName(this, "도면 저장",
            filename, "DXF 파일 (*.dxf)");
        if (path.isEmpty()) {
            message = "저장 취소됨";
        } else {
            DxfWriter writer(m_doc);
            auto result = writer.save(path.toStdString());
            if (result.success) {
                m_doc.setFilePath(path.toStdString());
                m_doc.setModified(false);
                message = "저장 완료: " + path;
            } else {
                message = "저장 실패: " + QString::fromStdString(result.errorMessage);
                success = false;
            }
        }
    }

    // Update UI
    if (success) {
        setStepStatus(stepIndex, message, "✅");
    } else {
        setStepStatus(stepIndex, message, "❌");
    }

    m_currentStep = stepIndex + 1;
    m_progressBar->setValue(m_currentStep);
    m_statusLabel->setText(message);

    if (m_canvas) {
        m_canvas->refreshEntities();
    }

    emit stepCompleted(stepIndex, message);
}

void DomusongPanel::onSkipStep() {
    if (m_currentStep >= static_cast<int>(m_steps.size())) return;
    setStepStatus(m_currentStep, "건너뜀", "⏭");
    ++m_currentStep;
    m_progressBar->setValue(m_currentStep);
    updateStepList();
}

} // namespace yjcad
