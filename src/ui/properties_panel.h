#pragma once

#include "core/document/document.h"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>

namespace yjcad {

class CadCanvas;

/// Properties panel — shows/edits selected entity attributes
class PropertiesPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertiesPanel(Document& doc, CadCanvas* canvas, QWidget* parent = nullptr);

public slots:
    void onSelectionChanged();

private slots:
    void onLayerChanged(int index);
    void onColorChanged();
    void onClassChanged(int index);
    void onApplyCoords();

private:
    void setupUI();
    void clearFields();
    void populateFromEntity(Entity* entity);

    Document& m_doc;
    CadCanvas* m_canvas;

    QLabel* m_titleLabel;
    QLabel* m_typeLabel;
    QLabel* m_idLabel;
    QComboBox* m_layerCombo;
    QLineEdit* m_colorEdit;
    QPushButton* m_colorBtn;
    QComboBox* m_classCombo;

    // Coordinate fields (context-dependent)
    QLineEdit* m_coord1Edit; // x1 or cx
    QLineEdit* m_coord2Edit; // y1 or cy
    QLineEdit* m_coord3Edit; // x2 or radius
    QLineEdit* m_coord4Edit; // y2
    QLabel* m_coord1Label;
    QLabel* m_coord2Label;
    QLabel* m_coord3Label;
    QLabel* m_coord4Label;
    QPushButton* m_applyBtn;

    EntityId m_currentEntityId = 0;
};

} // namespace yjcad
