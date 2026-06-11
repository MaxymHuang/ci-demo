#include "core/InspectionSession.h"

namespace xri {

InspectionSession::InspectionSession(QObject* parent)
    : QObject(parent)
    , m_roiModel(&m_plan)
{
}

void InspectionSession::setScanImage(const QImage& image, const ScanParams& params)
{
    m_scanImage = image;
    m_scanParams = params;

    // A new scan invalidates previous verdicts.
    m_hasResult = false;
    m_lastResult = {};
    m_roiModel.clearResults();

    emit scanChanged();
    emit resultsChanged();
}

const InspectionRunResult& InspectionSession::runInspection()
{
    m_lastResult = InspectionEngine::run(m_scanImage, m_plan);
    m_hasResult = true;
    m_roiModel.setResults(m_lastResult);
    emit resultsChanged();
    return m_lastResult;
}

} // namespace xri
