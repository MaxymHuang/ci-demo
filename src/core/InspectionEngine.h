#pragma once

#include "core/Algorithm.h"
#include "core/InspectionPlan.h"

#include <QDateTime>
#include <QUuid>

#include <vector>

namespace xri {

struct RoiResult {
    QUuid roiId;
    QString roiName;
    QString algorithmId;
    QString algorithmName;
    QRect rect; // clamped to image bounds, as evaluated
    QVariantMap params;
    bool evaluated = false; // false when no algorithm was assigned or ROI was empty
    AlgorithmResult result;
};

struct InspectionRunResult {
    QDateTime timestamp;
    std::vector<RoiResult> roiResults;
    bool overallPass = false;

    int evaluatedCount() const;
    int passCount() const;
};

class InspectionEngine
{
public:
    // Evaluates every ROI with an assigned algorithm. Overall verdict passes
    // when at least one ROI was evaluated and none failed.
    static InspectionRunResult run(const QImage& image, const InspectionPlan& plan);
};

} // namespace xri
