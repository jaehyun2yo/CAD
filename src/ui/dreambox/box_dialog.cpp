#include "box_dialog.h"
#include <QVBoxLayout>
#include <QLabel>

namespace yjcad {

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────

BoxDialog::BoxDialog(Document& doc, QWidget* parent)
    : QDialog(parent)
    , m_doc(doc)
{
    setWindowTitle(tr("박스 템플릿 생성"));
    setModal(true);
    setMinimumWidth(320);
    setupUI();
}

// ─────────────────────────────────────────────
// UI Setup
// ─────────────────────────────────────────────

void BoxDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // ── Box type selector ──
    auto* typeGroup = new QGroupBox(tr("박스 타입"), this);
    auto* typeLayout = new QVBoxLayout(typeGroup);
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("A형 맞뚜껑"));
    m_typeCombo->addItem(tr("접어넣기 (Tuck-end)"));
    m_typeCombo->addItem(tr("트레이"));
    m_typeCombo->addItem(tr("자동잠금"));
    m_typeCombo->addItem(tr("선물 박스 (상하 분리)"));
    typeLayout->addWidget(m_typeCombo);
    mainLayout->addWidget(typeGroup);

    // ── Parameter form ──
    auto* paramGroup = new QGroupBox(tr("치수 (mm)"), this);
    auto* form = new QFormLayout(paramGroup);

    auto makeSpinBox = [this](double min, double max, double val, double step) {
        auto* s = new QDoubleSpinBox(this);
        s->setRange(min, max);
        s->setValue(val);
        s->setSingleStep(step);
        s->setDecimals(1);
        s->setSuffix(" mm");
        return s;
    };

    m_widthSpin  = makeSpinBox(1.0, 5000.0, 100.0, 5.0);
    m_heightSpin = makeSpinBox(1.0, 5000.0,  80.0, 5.0);
    m_depthSpin  = makeSpinBox(1.0, 5000.0,  60.0, 5.0);
    m_thickSpin  = makeSpinBox(0.1,   10.0,   0.5, 0.1);

    form->addRow(tr("가로 (Width):"),        m_widthSpin);
    form->addRow(tr("세로 (Height):"),       m_heightSpin);
    form->addRow(tr("깊이/높이 (Depth):"),   m_depthSpin);
    form->addRow(tr("종이 두께 (Thickness):"), m_thickSpin);
    mainLayout->addWidget(paramGroup);

    // ── OK / Cancel ──
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &BoxDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoxDialog::onTypeChanged);
}

// ─────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────

void BoxDialog::onTypeChanged(int /*index*/)
{
    // Could update default values or hints per type in future
}

void BoxDialog::onAccepted()
{
    BoxTemplates::BoxParams p;
    p.width     = m_widthSpin->value();
    p.height    = m_heightSpin->value();
    p.depth     = m_depthSpin->value();
    p.thickness = m_thickSpin->value();

    BoxTemplates templates(m_doc);

    switch (static_cast<BoxType>(m_typeCombo->currentIndex())) {
    case ABox:     templates.createABox(p);     break;
    case TuckBox:  templates.createTuckBox(p);  break;
    case Tray:     templates.createTray(p);     break;
    case AutoLock: templates.createAutoLock(p); break;
    case GiftBox:  templates.createGiftBox(p);  break;
    }

    accept();
}

} // namespace yjcad
