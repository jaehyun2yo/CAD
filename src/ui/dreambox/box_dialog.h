#pragma once

#include "box_templates.h"
#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>

namespace yjcad {

/// BoxDialog — modal dialog for selecting a box template and entering its parameters.
///
/// Usage:
///   BoxDialog dlg(doc, parent);
///   if (dlg.exec() == QDialog::Accepted) {
///       // box entities have been added to doc
///   }
class BoxDialog : public QDialog {
    Q_OBJECT

public:
    explicit BoxDialog(Document& doc, QWidget* parent = nullptr);

private slots:
    void onAccepted();
    void onTypeChanged(int index);

private:
    void setupUI();

    Document& m_doc;

    // Widgets
    QComboBox*      m_typeCombo;
    QDoubleSpinBox* m_widthSpin;
    QDoubleSpinBox* m_heightSpin;
    QDoubleSpinBox* m_depthSpin;
    QDoubleSpinBox* m_thickSpin;
    QDialogButtonBox* m_buttonBox;

    /// Box type indices (must match m_typeCombo population order)
    enum BoxType {
        ABox     = 0,
        TuckBox  = 1,
        Tray     = 2,
        AutoLock = 3,
        GiftBox  = 4,
    };
};

} // namespace yjcad
