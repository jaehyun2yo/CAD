#include "laser_panel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QString>

namespace yjcad {

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────

LaserPanel::LaserPanel(Document& doc, CadCanvas* canvas, QWidget* parent)
    : QWidget(parent)
    , m_doc(doc)
    , m_canvas(canvas)
    , m_simTimer(new QTimer(this))
{
    setupUI();

    m_simTimer->setInterval(200); // 200 ms per entity step
    connect(m_simTimer, &QTimer::timeout, this, &LaserPanel::onSimulationTick);
}

// ─────────────────────────────────────────────
// UI Setup
// ─────────────────────────────────────────────

void LaserPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    // ── NC 설정 그룹 ──
    auto* ncGroup = new QGroupBox(tr("레이저 / NC 설정"), this);
    auto* ncForm  = new QFormLayout(ncGroup);

    m_feedRateSpin = new QDoubleSpinBox(this);
    m_feedRateSpin->setRange(1.0, 99999.0);
    m_feedRateSpin->setValue(100.0);
    m_feedRateSpin->setSingleStep(10.0);
    m_feedRateSpin->setDecimals(1);
    m_feedRateSpin->setSuffix(" mm/min");
    ncForm->addRow(tr("이송 속도:"), m_feedRateSpin);

    m_rapidRateSpin = new QDoubleSpinBox(this);
    m_rapidRateSpin->setRange(1.0, 99999.0);
    m_rapidRateSpin->setValue(3000.0);
    m_rapidRateSpin->setSingleStep(100.0);
    m_rapidRateSpin->setDecimals(1);
    m_rapidRateSpin->setSuffix(" mm/min");
    ncForm->addRow(tr("급이송 속도:"), m_rapidRateSpin);

    m_saveNcBtn = new QPushButton(tr("NC 파일 저장 (.nc)"), this);
    ncForm->addRow(m_saveNcBtn);

    mainLayout->addWidget(ncGroup);

    // ── 시뮬레이션 그룹 ──
    auto* simGroup = new QGroupBox(tr("경로 시뮬레이션"), this);
    auto* simLayout = new QVBoxLayout(simGroup);

    m_simStatusLabel = new QLabel(tr("준비"), this);
    m_simStatusLabel->setAlignment(Qt::AlignCenter);
    simLayout->addWidget(m_simStatusLabel);

    auto* btnRow = new QHBoxLayout;
    m_playBtn = new QPushButton(tr("재생"), this);
    m_stopBtn = new QPushButton(tr("중지"), this);
    m_stopBtn->setEnabled(false);
    btnRow->addWidget(m_playBtn);
    btnRow->addWidget(m_stopBtn);
    simLayout->addLayout(btnRow);

    mainLayout->addWidget(simGroup);
    mainLayout->addStretch();

    // ── Connections ──
    connect(m_saveNcBtn, &QPushButton::clicked, this, &LaserPanel::onSaveNc);
    connect(m_playBtn,   &QPushButton::clicked, this, &LaserPanel::onSimulatePlay);
    connect(m_stopBtn,   &QPushButton::clicked, this, &LaserPanel::onSimulateStop);
}

// ─────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────

void LaserPanel::onSaveNc()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("NC 파일 저장"),
        QString(),
        tr("NC Files (*.nc);;All Files (*)"));

    if (path.isEmpty()) return;

    NcGenerator gen;
    NcGenerator::NcConfig cfg;
    cfg.feedRate = m_feedRateSpin->value();

    const std::string gcode = gen.generate(m_doc, cfg);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("오류"),
            tr("파일을 저장할 수 없습니다:\n%1").arg(path));
        return;
    }

    QTextStream ts(&file);
    ts << QString::fromStdString(gcode);
    file.close();

    m_simStatusLabel->setText(tr("NC 저장 완료"));
}

void LaserPanel::onSimulatePlay()
{
    m_simEntityIds = m_doc.entityIds();
    if (m_simEntityIds.empty()) {
        m_simStatusLabel->setText(tr("엔티티 없음"));
        return;
    }

    m_simIndex  = 0;
    m_simPrevId = 0;
    updateSimulationState(true);
    m_simTimer->start();
}

void LaserPanel::onSimulateStop()
{
    m_simTimer->stop();

    // Deselect previously highlighted entity
    if (m_simPrevId != 0) {
        Entity* e = m_doc.entity(m_simPrevId);
        if (e) e->setSelected(false);
        m_simPrevId = 0;
    }

    updateSimulationState(false);
    m_simStatusLabel->setText(tr("중지됨"));
}

void LaserPanel::onSimulationTick()
{
    // Deselect previous
    if (m_simPrevId != 0) {
        Entity* e = m_doc.entity(m_simPrevId);
        if (e) e->setSelected(false);
    }

    if (m_simIndex >= static_cast<int>(m_simEntityIds.size())) {
        // Simulation complete
        m_simTimer->stop();
        m_simPrevId = 0;
        updateSimulationState(false);
        m_simStatusLabel->setText(tr("시뮬레이션 완료"));
        emit simulationFinished();
        return;
    }

    EntityId id = m_simEntityIds[m_simIndex];
    Entity* e = m_doc.entity(id);
    if (e && e->isVisible()) {
        e->setSelected(true);
        m_simPrevId = id;
        m_simStatusLabel->setText(
            tr("재생 중... %1 / %2")
                .arg(m_simIndex + 1)
                .arg(static_cast<int>(m_simEntityIds.size())));
        emit simulationStep(id);
    }

    ++m_simIndex;
}

// ─────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────

void LaserPanel::updateSimulationState(bool running)
{
    m_playBtn->setEnabled(!running);
    m_stopBtn->setEnabled(running);
    m_saveNcBtn->setEnabled(!running);
}

} // namespace yjcad
