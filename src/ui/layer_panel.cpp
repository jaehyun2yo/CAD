#include "layer_panel.h"
#include "ui/cad_canvas.h"
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

namespace yjcad {

LayerPanel::LayerPanel(Document& doc, CadCanvas* canvas, QWidget* parent)
    : QWidget(parent), m_doc(doc), m_canvas(canvas) {
    setupUI();
    refreshLayers();
}

void LayerPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* titleLabel = new QLabel("레이어 목록");
    titleLabel->setStyleSheet("color: #cccccc; font-weight: bold; font-size: 13px;");
    layout->addWidget(titleLabel);

    // Layer list with checkboxes
    m_layerList = new QListWidget();
    m_layerList->setStyleSheet(
        "QListWidget { background: #2b2b2b; color: #cccccc; border: 1px solid #555; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #4a90d9; }");
    layout->addWidget(m_layerList);

    connect(m_layerList, &QListWidget::itemChanged, this, &LayerPanel::onItemChanged);
    connect(m_layerList, &QListWidget::itemDoubleClicked, this, &LayerPanel::onItemDoubleClicked);

    // Add layer row
    auto* addRow = new QHBoxLayout();
    m_newLayerEdit = new QLineEdit();
    m_newLayerEdit->setPlaceholderText("새 레이어 이름");
    m_newLayerEdit->setStyleSheet("background: #3a3a3a; color: #cccccc; border: 1px solid #555;");
    m_addBtn = new QPushButton("+");
    m_addBtn->setFixedWidth(30);
    m_addBtn->setStyleSheet(
        "QPushButton { background: #4a90d9; color: white; border: none; border-radius: 3px; }"
        "QPushButton:hover { background: #5ba0e9; }");
    connect(m_addBtn, &QPushButton::clicked, this, &LayerPanel::onAddLayer);
    addRow->addWidget(m_newLayerEdit);
    addRow->addWidget(m_addBtn);
    layout->addLayout(addRow);

    // Action buttons
    auto* btnRow = new QHBoxLayout();
    m_setCurrentBtn = new QPushButton("현재 레이어로");
    m_setCurrentBtn->setStyleSheet(
        "QPushButton { background: #4a90d9; color: white; border: none; "
        "padding: 4px 8px; border-radius: 3px; }"
        "QPushButton:hover { background: #5ba0e9; }");
    m_removeBtn = new QPushButton("삭제");
    m_removeBtn->setStyleSheet(
        "QPushButton { background: #d94a4a; color: white; border: none; "
        "padding: 4px 8px; border-radius: 3px; }"
        "QPushButton:hover { background: #e95b5b; }");
    connect(m_setCurrentBtn, &QPushButton::clicked, this, &LayerPanel::onSetCurrentLayer);
    connect(m_removeBtn, &QPushButton::clicked, this, &LayerPanel::onRemoveLayer);
    btnRow->addWidget(m_setCurrentBtn);
    btnRow->addWidget(m_removeBtn);
    layout->addLayout(btnRow);
}

void LayerPanel::refreshLayers() {
    m_layerList->blockSignals(true);
    m_layerList->clear();

    auto layerNames = m_doc.layers().layerNames();
    std::string currentLayer = m_doc.layers().currentLayer();

    for (const auto& name : layerNames) {
        auto* layer = m_doc.layers().layer(name);
        if (!layer) continue;

        QString displayName = QString::fromStdString(name);
        if (name == currentLayer) displayName += " ★";

        auto* item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, QString::fromStdString(name));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(layer->isVisible() ? Qt::Checked : Qt::Unchecked);

        // Color indicator via icon (simplified: use foreground color)
        static const QColor aciColors[] = {
            Qt::red, Qt::yellow, Qt::green, Qt::cyan, Qt::blue,
            Qt::magenta, Qt::white, Qt::gray
        };
        int colorIdx = layer->color();
        QColor c = (colorIdx >= 1 && colorIdx <= 7) ? aciColors[colorIdx - 1] : Qt::white;
        item->setForeground(c);

        m_layerList->addItem(item);
    }

    m_layerList->blockSignals(false);
}

void LayerPanel::onItemChanged(QListWidgetItem* item) {
    if (!item) return;
    QString layerName = item->data(Qt::UserRole).toString();
    auto* layer = m_doc.layers().layer(layerName.toStdString());
    if (!layer) return;

    bool visible = (item->checkState() == Qt::Checked);
    layer->setVisible(visible);

    // Update entity visibility
    for (EntityId id : m_doc.entityIds()) {
        auto* entity = m_doc.entity(id);
        if (entity && entity->layerName() == layerName.toStdString()) {
            entity->setVisible(visible);
        }
    }

    if (m_canvas) m_canvas->refreshEntities();
}

void LayerPanel::onItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    QString layerName = item->data(Qt::UserRole).toString();
    auto* layer = m_doc.layers().layer(layerName.toStdString());
    if (!layer) return;

    QColor newColor = QColorDialog::getColor(Qt::white, this,
        QString("레이어 '%1' 색상").arg(layerName));
    if (!newColor.isValid()) return;

    // Map to simplified ACI color
    // For now, just set a basic color index
    layer->setColor(layer->color()); // Keep existing — full ACI mapping is complex
    item->setForeground(newColor);

    if (m_canvas) m_canvas->refreshEntities();
}

void LayerPanel::onAddLayer() {
    QString name = m_newLayerEdit->text().trimmed();
    if (name.isEmpty()) return;

    if (!m_doc.layers().addLayer(name.toStdString())) {
        QMessageBox::warning(this, "레이어", "이미 존재하는 레이어입니다.");
        return;
    }

    m_newLayerEdit->clear();
    refreshLayers();
}

void LayerPanel::onRemoveLayer() {
    auto* item = m_layerList->currentItem();
    if (!item) return;
    QString layerName = item->data(Qt::UserRole).toString();

    if (layerName == "0") {
        QMessageBox::warning(this, "레이어", "기본 레이어 '0'은 삭제할 수 없습니다.");
        return;
    }

    if (!m_doc.layers().removeLayer(layerName.toStdString())) {
        QMessageBox::warning(this, "레이어", "레이어를 삭제할 수 없습니다.");
        return;
    }

    refreshLayers();
    if (m_canvas) m_canvas->refreshEntities();
}

void LayerPanel::onSetCurrentLayer() {
    auto* item = m_layerList->currentItem();
    if (!item) return;
    QString layerName = item->data(Qt::UserRole).toString();
    m_doc.layers().setCurrentLayer(layerName.toStdString());
    refreshLayers();
}

} // namespace yjcad
