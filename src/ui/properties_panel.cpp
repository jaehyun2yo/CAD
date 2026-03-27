#include "properties_panel.h"
#include "ui/cad_canvas.h"
#include <QGroupBox>
#include <QColorDialog>

namespace yjcad {

static const char* classNames[] = {
    "Unknown", "오시 (Oshi)", "칼 (Knife)", "구아이 (Guide)",
    "브릿지 (Bridge)", "외곽 (Outline)"
};

PropertiesPanel::PropertiesPanel(Document& doc, CadCanvas* canvas, QWidget* parent)
    : QWidget(parent), m_doc(doc), m_canvas(canvas) {
    setupUI();
}

void PropertiesPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // Title
    m_titleLabel = new QLabel("선택 없음");
    m_titleLabel->setStyleSheet("color: #cccccc; font-weight: bold; font-size: 13px;");
    layout->addWidget(m_titleLabel);

    // Type & ID
    auto* infoLayout = new QFormLayout();
    infoLayout->setLabelAlignment(Qt::AlignRight);

    m_typeLabel = new QLabel("-");
    m_typeLabel->setStyleSheet("color: #aaaaaa;");
    m_idLabel = new QLabel("-");
    m_idLabel->setStyleSheet("color: #aaaaaa;");
    infoLayout->addRow("타입:", m_typeLabel);
    infoLayout->addRow("ID:", m_idLabel);
    layout->addLayout(infoLayout);

    // Layer
    auto* attrGroup = new QGroupBox("속성");
    attrGroup->setStyleSheet(
        "QGroupBox { color: #cccccc; border: 1px solid #555; border-radius: 4px; "
        "margin-top: 8px; padding-top: 14px; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }");
    auto* attrLayout = new QFormLayout(attrGroup);

    m_layerCombo = new QComboBox();
    m_layerCombo->setStyleSheet("background: #3a3a3a; color: #cccccc; border: 1px solid #555;");
    attrLayout->addRow("레이어:", m_layerCombo);
    connect(m_layerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertiesPanel::onLayerChanged);

    // Color
    auto* colorRow = new QHBoxLayout();
    m_colorEdit = new QLineEdit("7");
    m_colorEdit->setMaximumWidth(40);
    m_colorEdit->setStyleSheet("background: #3a3a3a; color: #cccccc; border: 1px solid #555;");
    m_colorBtn = new QPushButton();
    m_colorBtn->setFixedSize(24, 24);
    m_colorBtn->setStyleSheet("background: white; border: 1px solid #555;");
    connect(m_colorBtn, &QPushButton::clicked, this, &PropertiesPanel::onColorChanged);
    colorRow->addWidget(m_colorEdit);
    colorRow->addWidget(m_colorBtn);
    colorRow->addStretch();
    attrLayout->addRow("색상:", colorRow);

    // Classification
    m_classCombo = new QComboBox();
    m_classCombo->setStyleSheet("background: #3a3a3a; color: #cccccc; border: 1px solid #555;");
    for (int i = 0; i < 6; ++i)
        m_classCombo->addItem(classNames[i]);
    attrLayout->addRow("분류:", m_classCombo);
    connect(m_classCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertiesPanel::onClassChanged);

    layout->addWidget(attrGroup);

    // Coordinates
    auto* coordGroup = new QGroupBox("좌표");
    coordGroup->setStyleSheet(attrGroup->styleSheet());
    auto* coordLayout = new QFormLayout(coordGroup);

    m_coord1Label = new QLabel("X1:");
    m_coord2Label = new QLabel("Y1:");
    m_coord3Label = new QLabel("X2:");
    m_coord4Label = new QLabel("Y2:");

    auto makeCoordEdit = [this]() {
        auto* edit = new QLineEdit();
        edit->setStyleSheet("background: #3a3a3a; color: #cccccc; border: 1px solid #555;");
        return edit;
    };
    m_coord1Edit = makeCoordEdit();
    m_coord2Edit = makeCoordEdit();
    m_coord3Edit = makeCoordEdit();
    m_coord4Edit = makeCoordEdit();

    coordLayout->addRow(m_coord1Label, m_coord1Edit);
    coordLayout->addRow(m_coord2Label, m_coord2Edit);
    coordLayout->addRow(m_coord3Label, m_coord3Edit);
    coordLayout->addRow(m_coord4Label, m_coord4Edit);

    m_applyBtn = new QPushButton("적용");
    m_applyBtn->setStyleSheet(
        "QPushButton { background: #4a90d9; color: white; border: none; "
        "padding: 4px 12px; border-radius: 3px; }"
        "QPushButton:hover { background: #5ba0e9; }");
    connect(m_applyBtn, &QPushButton::clicked, this, &PropertiesPanel::onApplyCoords);
    coordLayout->addRow("", m_applyBtn);

    layout->addWidget(coordGroup);
    layout->addStretch();

    clearFields();
}

void PropertiesPanel::clearFields() {
    m_titleLabel->setText("선택 없음");
    m_typeLabel->setText("-");
    m_idLabel->setText("-");
    m_layerCombo->clear();
    m_colorEdit->setText("");
    m_classCombo->setCurrentIndex(0);
    m_coord1Edit->clear();
    m_coord2Edit->clear();
    m_coord3Edit->clear();
    m_coord4Edit->clear();
    m_currentEntityId = 0;
    setEnabled(false);
}

void PropertiesPanel::onSelectionChanged() {
    auto selected = m_doc.selection().selectedIds();
    if (selected.empty()) {
        clearFields();
        return;
    }

    // Show first selected entity
    EntityId id = *selected.begin();
    Entity* entity = m_doc.entity(id);
    if (!entity) { clearFields(); return; }

    setEnabled(true);
    populateFromEntity(entity);
}

void PropertiesPanel::populateFromEntity(Entity* entity) {
    m_currentEntityId = entity->id();

    // Type name
    static const char* typeNames[] = {
        "선 (Line)", "원 (Circle)", "호 (Arc)", "폴리라인", "텍스트", "삽입", "점"
    };
    int typeIdx = static_cast<int>(entity->type());
    m_typeLabel->setText(typeNames[typeIdx]);
    m_idLabel->setText(QString::number(entity->id()));
    m_titleLabel->setText(QString("엔티티 #%1").arg(entity->id()));

    // Layer combo
    m_layerCombo->blockSignals(true);
    m_layerCombo->clear();
    auto layerNames = m_doc.layers().layerNames();
    int currentIdx = 0;
    for (size_t i = 0; i < layerNames.size(); ++i) {
        m_layerCombo->addItem(QString::fromStdString(layerNames[i]));
        if (layerNames[i] == entity->layerName()) currentIdx = static_cast<int>(i);
    }
    m_layerCombo->setCurrentIndex(currentIdx);
    m_layerCombo->blockSignals(false);

    // Color
    m_colorEdit->setText(QString::number(entity->color()));

    // Classification
    m_classCombo->blockSignals(true);
    m_classCombo->setCurrentIndex(static_cast<int>(entity->classification()));
    m_classCombo->blockSignals(false);

    // Coordinates depend on type
    switch (entity->type()) {
    case EntityType::Line: {
        auto* line = static_cast<LineEntity*>(entity);
        m_coord1Label->setText("X1:"); m_coord1Edit->setText(QString::number(line->start().x, 'f', 3));
        m_coord2Label->setText("Y1:"); m_coord2Edit->setText(QString::number(line->start().y, 'f', 3));
        m_coord3Label->setText("X2:"); m_coord3Edit->setText(QString::number(line->end().x, 'f', 3));
        m_coord4Label->setText("Y2:"); m_coord4Edit->setText(QString::number(line->end().y, 'f', 3));
        m_coord3Edit->setVisible(true); m_coord3Label->setVisible(true);
        m_coord4Edit->setVisible(true); m_coord4Label->setVisible(true);
        break;
    }
    case EntityType::Circle: {
        auto* circle = static_cast<CircleEntity*>(entity);
        m_coord1Label->setText("CX:"); m_coord1Edit->setText(QString::number(circle->center().x, 'f', 3));
        m_coord2Label->setText("CY:"); m_coord2Edit->setText(QString::number(circle->center().y, 'f', 3));
        m_coord3Label->setText("반지름:"); m_coord3Edit->setText(QString::number(circle->radius(), 'f', 3));
        m_coord4Label->setText(""); m_coord4Edit->setVisible(false); m_coord4Label->setVisible(false);
        m_coord3Edit->setVisible(true); m_coord3Label->setVisible(true);
        break;
    }
    case EntityType::Arc: {
        auto* arc = static_cast<ArcEntity*>(entity);
        m_coord1Label->setText("CX:"); m_coord1Edit->setText(QString::number(arc->center().x, 'f', 3));
        m_coord2Label->setText("CY:"); m_coord2Edit->setText(QString::number(arc->center().y, 'f', 3));
        m_coord3Label->setText("반지름:"); m_coord3Edit->setText(QString::number(arc->radius(), 'f', 3));
        m_coord4Label->setText(""); m_coord4Edit->setVisible(false); m_coord4Label->setVisible(false);
        m_coord3Edit->setVisible(true); m_coord3Label->setVisible(true);
        break;
    }
    default:
        m_coord1Label->setText("X:"); m_coord1Edit->clear();
        m_coord2Label->setText("Y:"); m_coord2Edit->clear();
        m_coord3Edit->setVisible(false); m_coord3Label->setVisible(false);
        m_coord4Edit->setVisible(false); m_coord4Label->setVisible(false);
        break;
    }
}

void PropertiesPanel::onLayerChanged(int index) {
    if (m_currentEntityId == 0 || index < 0) return;
    auto* entity = m_doc.entity(m_currentEntityId);
    if (!entity) return;
    entity->setLayerName(m_layerCombo->currentText().toStdString());
    m_doc.setModified(true);
    if (m_canvas) m_canvas->refreshEntities();
}

void PropertiesPanel::onColorChanged() {
    if (m_currentEntityId == 0) return;
    auto* entity = m_doc.entity(m_currentEntityId);
    if (!entity) return;

    QColor color = QColorDialog::getColor(Qt::white, this, "엔티티 색상");
    if (!color.isValid()) return;

    // Store as AutoCAD color index approximation (simplified)
    int colorIdx = m_colorEdit->text().toInt();
    entity->setColor(colorIdx);
    m_colorBtn->setStyleSheet(
        QString("background: %1; border: 1px solid #555;").arg(color.name()));
    m_doc.setModified(true);
    if (m_canvas) m_canvas->refreshEntities();
}

void PropertiesPanel::onClassChanged(int index) {
    if (m_currentEntityId == 0 || index < 0) return;
    auto* entity = m_doc.entity(m_currentEntityId);
    if (!entity) return;
    entity->setClassification(static_cast<LineClassification>(index));
    m_doc.setModified(true);
    if (m_canvas) m_canvas->refreshEntities();
}

void PropertiesPanel::onApplyCoords() {
    if (m_currentEntityId == 0) return;
    auto* entity = m_doc.entity(m_currentEntityId);
    if (!entity) return;

    switch (entity->type()) {
    case EntityType::Line: {
        auto* line = static_cast<LineEntity*>(entity);
        bool ok1, ok2, ok3, ok4;
        double x1 = m_coord1Edit->text().toDouble(&ok1);
        double y1 = m_coord2Edit->text().toDouble(&ok2);
        double x2 = m_coord3Edit->text().toDouble(&ok3);
        double y2 = m_coord4Edit->text().toDouble(&ok4);
        if (ok1 && ok2 && ok3 && ok4) {
            line->setStart({x1, y1});
            line->setEnd({x2, y2});
        }
        break;
    }
    case EntityType::Circle: {
        auto* circle = static_cast<CircleEntity*>(entity);
        bool ok1, ok2, ok3;
        double cx = m_coord1Edit->text().toDouble(&ok1);
        double cy = m_coord2Edit->text().toDouble(&ok2);
        double r = m_coord3Edit->text().toDouble(&ok3);
        if (ok1 && ok2 && ok3 && r > 0) {
            circle->setCenter({cx, cy});
            circle->setRadius(r);
        }
        break;
    }
    default:
        break;
    }

    m_doc.setModified(true);
    if (m_canvas) m_canvas->refreshEntities();
}

} // namespace yjcad
