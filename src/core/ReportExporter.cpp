#include "core/ReportExporter.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>

namespace xri {

namespace {

QString verdictText(const RoiResult& r)
{
    if (!r.evaluated)
        return QStringLiteral("N/A");
    return r.result.pass ? QStringLiteral("PASS") : QStringLiteral("FAIL");
}

QJsonObject resultToJson(const ScanParams& scanParams, const InspectionRunResult& result)
{
    QJsonArray rois;
    for (const RoiResult& r : result.roiResults) {
        rois.append(QJsonObject{
            { "name", r.roiName },
            { "rect",
              QJsonObject{ { "x", r.rect.x() },
                           { "y", r.rect.y() },
                           { "w", r.rect.width() },
                           { "h", r.rect.height() } } },
            { "algorithm", r.algorithmId },
            { "algorithmName", r.algorithmName },
            { "params", QJsonObject::fromVariantMap(r.params) },
            { "evaluated", r.evaluated },
            { "measuredValue", r.result.measuredValue },
            { "unit", r.result.unit },
            { "pass", r.result.pass },
            { "detail", r.result.detail },
        });
    }
    return {
        { "generatedAt", result.timestamp.toString(Qt::ISODate) },
        { "scan",
          QJsonObject{ { "width", scanParams.width },
                       { "height", scanParams.height },
                       { "seed", static_cast<qint64>(scanParams.seed) },
                       { "objectCount", scanParams.objectCount },
                       { "noiseSigma", scanParams.noiseSigma },
                       { "addDefect", scanParams.addDefect } } },
        { "overallPass", result.overallPass },
        { "evaluatedRois", result.evaluatedCount() },
        { "passedRois", result.passCount() },
        { "rois", rois },
    };
}

QString resultToCsv(const InspectionRunResult& result)
{
    QString csv = QStringLiteral("roi,algorithm,measured,unit,verdict,detail,x,y,w,h\n");
    for (const RoiResult& r : result.roiResults) {
        csv += QStringLiteral("\"%1\",%2,%3,%4,%5,\"%6\",%7,%8,%9,%10\n")
                   .arg(r.roiName, r.algorithmId)
                   .arg(r.result.measuredValue, 0, 'f', 2)
                   .arg(r.result.unit, verdictText(r), r.result.detail)
                   .arg(r.rect.x()).arg(r.rect.y()).arg(r.rect.width()).arg(r.rect.height());
    }
    return csv;
}

QString resultToHtml(const QString& annotatedPngName, const ScanParams& scanParams,
                     const InspectionRunResult& result)
{
    QString rows;
    for (const RoiResult& r : result.roiResults) {
        const QString color = !r.evaluated ? QStringLiteral("#8b949e")
            : r.result.pass               ? QStringLiteral("#3fb950")
                                          : QStringLiteral("#f85149");
        rows += QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3 %4</td>"
                               "<td style=\"color:%5;font-weight:bold\">%6</td><td>%7</td></tr>\n")
                    .arg(r.roiName.toHtmlEscaped(),
                         r.algorithmName.isEmpty() ? QStringLiteral("—") : r.algorithmName,
                         r.evaluated ? QString::number(r.result.measuredValue, 'f', 2)
                                     : QStringLiteral("—"),
                         r.result.unit, color, verdictText(r), r.result.detail.toHtmlEscaped());
    }

    const QString overall = result.overallPass
        ? QStringLiteral("<span style=\"color:#3fb950\">PASS</span>")
        : QStringLiteral("<span style=\"color:#f85149\">FAIL</span>");

    return QStringLiteral(
               "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
               "<title>X-Ray Inspection Report</title>"
               "<style>body{font-family:sans-serif;background:#0d1117;color:#e6edf3;"
               "margin:2em}table{border-collapse:collapse}td,th{border:1px solid #30363d;"
               "padding:6px 12px}img{max-width:100%;border:1px solid #30363d}</style>"
               "</head><body>"
               "<h1>X-Ray Inspection Report</h1>"
               "<p>Generated: %1 &nbsp;|&nbsp; Scan: %2&times;%3, seed %4 &nbsp;|&nbsp; "
               "Overall: %5 (%6/%7 passed)</p>"
               "<img src=\"%8\" alt=\"annotated scan\">"
               "<h2>Results</h2>"
               "<table><tr><th>ROI</th><th>Algorithm</th><th>Measured</th>"
               "<th>Verdict</th><th>Detail</th></tr>%9</table>"
               "</body></html>\n")
        .arg(result.timestamp.toString(Qt::ISODate))
        .arg(scanParams.width).arg(scanParams.height).arg(scanParams.seed)
        .arg(overall).arg(result.passCount()).arg(result.evaluatedCount())
        .arg(annotatedPngName, rows);
}

bool writeTextFile(const QString& path, const QString& content, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *error = QStringLiteral("Cannot write %1: %2").arg(path, file.errorString());
        return false;
    }
    file.write(content.toUtf8());
    return true;
}

} // namespace

QImage ReportExporter::annotatedImage(const QImage& image, const InspectionRunResult& result)
{
    QImage annotated = image.convertToFormat(QImage::Format_RGB32);
    QPainter painter(&annotated);
    QFont font = painter.font();
    font.setPointSizeF(9.0);
    font.setBold(true);
    painter.setFont(font);

    for (const RoiResult& r : result.roiResults) {
        const QColor color = !r.evaluated ? QColor(QStringLiteral("#8b949e"))
            : r.result.pass               ? QColor(QStringLiteral("#3fb950"))
                                          : QColor(QStringLiteral("#f85149"));
        painter.setPen(QPen(color, 2));
        painter.drawRect(r.rect);

        QString label = r.roiName;
        if (r.evaluated)
            label += QStringLiteral(": %1 %2 (%3)")
                         .arg(r.result.measuredValue, 0, 'f', 1)
                         .arg(r.result.unit, verdictText(r));
        painter.drawText(r.rect.adjusted(3, 1, 200, 40).topLeft() + QPoint(0, 12), label);
    }
    return annotated;
}

ReportExporter::Outcome ReportExporter::exportAll(const QString& dirPath, const QString& baseName,
                                                  const QImage& image,
                                                  const ScanParams& scanParams,
                                                  const InspectionRunResult& result)
{
    Outcome outcome;
    if (image.isNull()) {
        outcome.error = QStringLiteral("No scan image to export");
        return outcome;
    }

    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        outcome.error = QStringLiteral("Cannot create directory %1").arg(dirPath);
        return outcome;
    }

    const QString rawPath = dir.filePath(baseName + QStringLiteral(".png"));
    if (!image.save(rawPath)) {
        outcome.error = QStringLiteral("Cannot write %1").arg(rawPath);
        return outcome;
    }
    outcome.writtenFiles << rawPath;

    const QString annotatedName = baseName + QStringLiteral("_annotated.png");
    const QString annotatedPath = dir.filePath(annotatedName);
    if (!annotatedImage(image, result).save(annotatedPath)) {
        outcome.error = QStringLiteral("Cannot write %1").arg(annotatedPath);
        return outcome;
    }
    outcome.writtenFiles << annotatedPath;

    const QString jsonPath = dir.filePath(baseName + QStringLiteral("_report.json"));
    const QJsonDocument doc(resultToJson(scanParams, result));
    if (!writeTextFile(jsonPath, QString::fromUtf8(doc.toJson(QJsonDocument::Indented)),
                       &outcome.error))
        return outcome;
    outcome.writtenFiles << jsonPath;

    const QString csvPath = dir.filePath(baseName + QStringLiteral("_report.csv"));
    if (!writeTextFile(csvPath, resultToCsv(result), &outcome.error))
        return outcome;
    outcome.writtenFiles << csvPath;

    const QString htmlPath = dir.filePath(baseName + QStringLiteral("_report.html"));
    if (!writeTextFile(htmlPath, resultToHtml(annotatedName, scanParams, result), &outcome.error))
        return outcome;
    outcome.writtenFiles << htmlPath;

    outcome.ok = true;
    return outcome;
}

} // namespace xri
