#pragma once

#include "core/InspectionSession.h"

#include <QUuid>
#include <QWidget>

class QComboBox;
class QFormLayout;
class QLabel;
class QListView;
class QPushButton;

namespace xri {

// Assign algorithms and parameters to inspection boxes, run the inspection
// and export the results.
class AlgorithmsPage : public QWidget
{
    Q_OBJECT
public:
    explicit AlgorithmsPage(InspectionSession* session, QWidget* parent = nullptr);

signals:
    void statusMessage(const QString& message);

private:
    QUuid selectedRoiId() const;
    void onRoiSelectionChanged();
    void onAlgorithmChanged(int comboIndex);
    void rebuildParamForm();
    void writeParam(const QString& key, double value);
    void runInspection();
    void exportResults();
    void updateOverallLabel();

    InspectionSession* m_session;
    QListView* m_roiList;
    QComboBox* m_algorithmCombo;
    QWidget* m_paramContainer;
    QFormLayout* m_paramForm;
    QLabel* m_selectionLabel;
    QLabel* m_overallLabel;
    QPushButton* m_runButton;
    QPushButton* m_exportButton;
    bool m_loading = false; // guards combo/param writes while loading a ROI
};

} // namespace xri
