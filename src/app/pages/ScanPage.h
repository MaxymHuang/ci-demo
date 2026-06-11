#pragma once

#include "core/InspectionSession.h"
#include "core/ScanSimulator.h"

#include <QWidget>

class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QSpinBox;

namespace xri {

// Mock 2D acquisition: parameter form, progress bar and a live preview that
// fills in band-by-band.
class ScanPage : public QWidget
{
    Q_OBJECT
public:
    explicit ScanPage(InspectionSession* session, QWidget* parent = nullptr);

    ScanSimulator* simulator() { return &m_simulator; }

signals:
    void statusMessage(const QString& message);
    void scanCompleted();

private:
    void startScan();
    void showPreview(const QImage& image);

    InspectionSession* m_session;
    ScanSimulator m_simulator;
    ScanParams m_currentParams;

    QSpinBox* m_seedSpin;
    QSpinBox* m_objectCountSpin;
    QCheckBox* m_defectCheck;
    QPushButton* m_startButton;
    QProgressBar* m_progressBar;
    QLabel* m_preview;
};

} // namespace xri
