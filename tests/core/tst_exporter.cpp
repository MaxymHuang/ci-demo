#include "core/InspectionEngine.h"
#include "core/InspectionPlan.h"
#include "core/ReportExporter.h"
#include "core/SyntheticXraySource.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

using namespace xri;

class TestExporter : public QObject
{
    Q_OBJECT
private slots:
    void exportsAllFormats()
    {
        ScanParams params;
        params.width = 160;
        params.height = 120;
        const QImage image = SyntheticXraySource::generate(params);

        InspectionPlan plan;
        const QUuid id = plan.addRoi(QRectF(10, 10, 60, 50));
        Roi roi = *plan.roiById(id);
        roi.algorithmId = QStringLiteral("mean_intensity");
        plan.updateRoi(roi);
        plan.addRoi(QRectF(80, 60, 40, 30)); // unassigned

        const InspectionRunResult result = InspectionEngine::run(image, plan);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const ReportExporter::Outcome outcome =
            ReportExporter::exportAll(dir.path(), QStringLiteral("run1"), image, params, result);

        QVERIFY2(outcome.ok, qPrintable(outcome.error));
        QCOMPARE(outcome.writtenFiles.size(), 5);
        for (const QString& file : outcome.writtenFiles)
            QVERIFY2(QFile::exists(file), qPrintable(file));

        // JSON is machine-readable and complete.
        QFile json(dir.filePath(QStringLiteral("run1_report.json")));
        QVERIFY(json.open(QIODevice::ReadOnly));
        const QJsonObject root = QJsonDocument::fromJson(json.readAll()).object();
        QCOMPARE(root.value("rois").toArray().size(), 2);
        QCOMPARE(root.value("overallPass").toBool(), result.overallPass);
        QCOMPARE(root.value("scan").toObject().value("width").toInt(), 160);
        const QJsonObject firstRoi = root.value("rois").toArray().first().toObject();
        QCOMPARE(firstRoi.value("algorithm").toString(), QStringLiteral("mean_intensity"));
        QVERIFY(firstRoi.value("evaluated").toBool());

        // Annotated image differs from the raw scan (overlay was painted).
        const QImage raw(dir.filePath(QStringLiteral("run1.png")));
        const QImage annotated(dir.filePath(QStringLiteral("run1_annotated.png")));
        QCOMPARE(raw.size(), annotated.size());
        QVERIFY(raw.convertToFormat(QImage::Format_RGB32) != annotated);

        // CSV has a header plus one row per ROI.
        QFile csv(dir.filePath(QStringLiteral("run1_report.csv")));
        QVERIFY(csv.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(csv.readAll()).trimmed().split('\n').size(), 3);
    }

    void failsCleanlyWithoutImage()
    {
        const ReportExporter::Outcome outcome = ReportExporter::exportAll(
            QDir::tempPath(), QStringLiteral("x"), QImage(), ScanParams{}, InspectionRunResult{});
        QVERIFY(!outcome.ok);
        QVERIFY(!outcome.error.isEmpty());
        QVERIFY(outcome.writtenFiles.isEmpty());
    }
};

QTEST_MAIN(TestExporter) // QPainter text rendering needs a QGuiApplication
#include "tst_exporter.moc"
