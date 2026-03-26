#pragma once

#include "core/document/document.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QGroupBox>
#include <vector>

namespace yjcad {

class CadCanvas;

/// 도무송 파이프라인 패널 — 오른쪽 도킹 패널의 "도무송" 탭
class DomusongPanel : public QWidget {
    Q_OBJECT

public:
    explicit DomusongPanel(Document& doc, CadCanvas* canvas, QWidget* parent = nullptr);

signals:
    void pipelineCompleted();
    void stepCompleted(int stepIndex, const QString& message);

private slots:
    void onRunPipeline();
    void onRunSingleStep(int stepIndex);
    void onSkipStep();

private:
    void setupUI();
    void updateStepList();
    void setStepStatus(int index, const QString& status, const QString& icon);

    Document& m_doc;
    CadCanvas* m_canvas;

    // UI elements
    QGroupBox* m_configGroup;
    QLineEdit* m_companyEdit;
    QLineEdit* m_drawingNameEdit;
    QComboBox* m_jeolsuCombo;
    QComboBox* m_printSizeCombo;
    QComboBox* m_orientationCombo;

    QPushButton* m_runAllBtn;
    QPushButton* m_runStepBtn;
    QPushButton* m_skipBtn;
    QProgressBar* m_progressBar;
    QListWidget* m_stepList;
    QLabel* m_statusLabel;

    int m_currentStep = 0;

    struct StepInfo {
        QString name;
        QString description;
        bool requiresConfirm;
    };
    std::vector<StepInfo> m_steps;
};

} // namespace yjcad
