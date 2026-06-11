#include "core/InspectionEngine.h"

#include "core/AlgorithmRegistry.h"

namespace xri {

int InspectionRunResult::evaluatedCount() const
{
    int count = 0;
    for (const RoiResult& r : roiResults)
        count += r.evaluated ? 1 : 0;
    return count;
}

int InspectionRunResult::passCount() const
{
    int count = 0;
    for (const RoiResult& r : roiResults)
        count += (r.evaluated && r.result.pass) ? 1 : 0;
    return count;
}

InspectionRunResult InspectionEngine::run(const QImage& image, const InspectionPlan& plan)
{
    InspectionRunResult run;
    run.timestamp = QDateTime::currentDateTime();

    const QRect imageRect(QPoint(0, 0), image.size());
    bool anyFail = false;

    for (const Roi& roi : plan.rois()) {
        RoiResult r;
        r.roiId = roi.id;
        r.roiName = roi.name;
        r.algorithmId = roi.algorithmId;
        r.params = roi.params;
        r.rect = roi.rect.toRect().intersected(imageRect);

        const IAlgorithm* algorithm = AlgorithmRegistry::instance().byId(roi.algorithmId);
        if (algorithm && !r.rect.isEmpty() && !image.isNull()) {
            r.algorithmName = algorithm->displayName();
            r.result = algorithm->evaluate(image, r.rect, roi.params);
            r.evaluated = true;
            anyFail = anyFail || !r.result.pass;
        }
        run.roiResults.push_back(r);
    }

    run.overallPass = run.evaluatedCount() > 0 && !anyFail;
    return run;
}

} // namespace xri
