#pragma once

#include "core/document/document.h"
#include "core/cam/nc_generator.h"
#include <QWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <vector>

namespace yjcad {

class CadCanvas;

/// LaserPanel — right-dock panel for laser/NC output and simulation.
///
/// Features:
///   - Feed rate control
///   - NC G-code export to file
///   - Entity-by-entity simulation playback (highlights entities in order)
class LaserPanel : public QWidget {
    Q_OBJECT

public:
    explicit LaserPanel(Document& doc, CadCanvas* canvas, QWidget* parent = nullptr);

signals:
    /// Emitted when simulation highlights entity with given ID.
    void simulationStep(EntityId id);

    /// Emitted when simulation completes.
    void simulationFinished();

private slots:
    void onSaveNc();
    void onSimulatePlay();
    void onSimulateStop();
    void onSimulationTick();

private:
    void setupUI();
    void updateSimulationState(bool running);

    Document&   m_doc;
    CadCanvas*  m_canvas;

    // ── NC Config widgets ──
    QDoubleSpinBox* m_feedRateSpin;
    QDoubleSpinBox* m_rapidRateSpin; // for future use
    QPushButton*    m_saveNcBtn;

    // ── Simulation widgets ──
    QPushButton* m_playBtn;
    QPushButton* m_stopBtn;
    QLabel*      m_simStatusLabel;

    // ── Simulation state ──
    QTimer*              m_simTimer;
    std::vector<EntityId> m_simEntityIds;
    int                  m_simIndex = 0;
    EntityId             m_simPrevId = 0; // last highlighted entity
};

} // namespace yjcad
