#pragma once

#include "core/InspectionEngine.h"
#include "core/InspectionPlan.h"
#include "core/RoiListModel.h"
#include "core/SyntheticXraySource.h"

#include <QImage>
#include <QObject>

namespace xri {

// Shared application state: the current scan, the inspection plan and the
// latest run result. Pages observe this object instead of each other.
class InspectionSession : public QObject
{
    Q_OBJECT
public:
    explicit InspectionSession(QObject* parent = nullptr);

    bool hasScan() const { return !m_scanImage.isNull(); }
    const QImage& scanImage() const { return m_scanImage; }
    const ScanParams& scanParams() const { return m_scanParams; }
    void setScanImage(const QImage& image, const ScanParams& params);

    InspectionPlan* plan() { return &m_plan; }
    const InspectionPlan* plan() const { return &m_plan; }
    RoiListModel* roiModel() { return &m_roiModel; }

    bool hasResult() const { return m_hasResult; }
    const InspectionRunResult& lastResult() const { return m_lastResult; }
    const InspectionRunResult& runInspection();

signals:
    void scanChanged();
    void resultsChanged();

private:
    QImage m_scanImage;
    ScanParams m_scanParams;
    InspectionPlan m_plan;
    RoiListModel m_roiModel;
    InspectionRunResult m_lastResult;
    bool m_hasResult = false;
};

} // namespace xri
