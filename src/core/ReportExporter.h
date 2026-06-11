#pragma once

#include "core/InspectionEngine.h"
#include "core/SyntheticXraySource.h"

#include <QImage>
#include <QString>
#include <QStringList>

namespace xri {

// Writes the inspection deliverables: raw + annotated x-ray PNGs and a
// JSON / CSV / HTML report.
class ReportExporter
{
public:
    struct Outcome {
        bool ok = false;
        QStringList writtenFiles;
        QString error;
    };

    static Outcome exportAll(const QString& dirPath, const QString& baseName,
                             const QImage& image, const ScanParams& scanParams,
                             const InspectionRunResult& result);

    // ROI overlay: green = pass, red = fail, gray = not evaluated.
    // Needs a QGuiApplication (QPainter text rendering).
    static QImage annotatedImage(const QImage& image, const InspectionRunResult& result);
};

} // namespace xri
