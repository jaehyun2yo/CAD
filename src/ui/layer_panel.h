#pragma once

#include "core/document/document.h"
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>

namespace yjcad {

class CadCanvas;

/// Layer panel — lists layers with visibility toggle and color control
class LayerPanel : public QWidget {
    Q_OBJECT

public:
    explicit LayerPanel(Document& doc, CadCanvas* canvas, QWidget* parent = nullptr);

public slots:
    void refreshLayers();

private slots:
    void onItemChanged(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onAddLayer();
    void onRemoveLayer();
    void onSetCurrentLayer();

private:
    void setupUI();

    Document& m_doc;
    CadCanvas* m_canvas;

    QListWidget* m_layerList;
    QLineEdit* m_newLayerEdit;
    QPushButton* m_addBtn;
    QPushButton* m_removeBtn;
    QPushButton* m_setCurrentBtn;
};

} // namespace yjcad
