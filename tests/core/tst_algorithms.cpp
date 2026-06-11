#include "core/AlgorithmRegistry.h"
#include "core/InspectionEngine.h"
#include "core/InspectionPlan.h"
#include "core/SyntheticXraySource.h"

#include <QTest>

using namespace xri;

namespace {

QImage uniformImage(int value, int w = 100, int h = 100)
{
    QImage img(w, h, QImage::Format_Grayscale8);
    img.fill(value);
    return img;
}

void fillRect(QImage& img, const QRect& rect, int value)
{
    for (int y = rect.top(); y <= rect.bottom(); ++y) {
        uchar* line = img.scanLine(y);
        for (int x = rect.left(); x <= rect.right(); ++x)
            line[x] = uchar(value);
    }
}

const IAlgorithm* algo(const char* id)
{
    const IAlgorithm* a = AlgorithmRegistry::instance().byId(QLatin1String(id));
    Q_ASSERT(a);
    return a;
}

} // namespace

class TestAlgorithms : public QObject
{
    Q_OBJECT
private slots:
    void registryKnowsAllAlgorithms()
    {
        const auto& registry = AlgorithmRegistry::instance();
        QCOMPARE(registry.all().size(), 4);
        for (const IAlgorithm* a : registry.all()) {
            QCOMPARE(registry.byId(a->id()), a);
            QVERIFY(!a->displayName().isEmpty());
            QVERIFY(!a->paramSpecs().isEmpty());
        }
        QCOMPARE(registry.byId(QStringLiteral("does_not_exist")), nullptr);
    }

    void meanIntensityMeasuresExactly()
    {
        const QImage img = uniformImage(100);
        const QRect roi(10, 10, 50, 50);

        AlgorithmResult r = algo("mean_intensity")->evaluate(img, roi, {});
        QCOMPARE(r.measuredValue, 100.0);
        QVERIFY(r.pass); // defaults [60, 220]

        r = algo("mean_intensity")->evaluate(img, roi, { { "minMean", 150.0 } });
        QVERIFY(!r.pass);
        r = algo("mean_intensity")->evaluate(img, roi,
                                             { { "minMean", 100.0 }, { "maxMean", 100.0 } });
        QVERIFY(r.pass);
    }

    void densityRangeFindsOutliers()
    {
        QImage img = uniformImage(100);
        fillRect(img, QRect(20, 20, 5, 5), 10); // dark outlier patch
        const QRect roi(0, 0, 100, 100);

        AlgorithmResult r = algo("density_range")->evaluate(img, roi, { { "minAllowed", 20.0 } });
        QCOMPARE(r.measuredValue, 10.0);
        QVERIFY(!r.pass);

        r = algo("density_range")->evaluate(img, roi, { { "minAllowed", 5.0 } });
        QVERIFY(r.pass);

        // ROI not covering the patch never sees it.
        r = algo("density_range")->evaluate(img, QRect(50, 50, 40, 40),
                                            { { "minAllowed", 20.0 } });
        QVERIFY(r.pass);
    }

    void blobCountCountsDarkComponents()
    {
        QImage img = uniformImage(200);
        fillRect(img, QRect(10, 10, 10, 10), 10); // blob 1: 100 px
        fillRect(img, QRect(40, 40, 10, 10), 10); // blob 2: 100 px
        fillRect(img, QRect(70, 70, 2, 2), 10);   // 4 px: below min area
        const QRect roi(0, 0, 100, 100);

        AlgorithmResult r = algo("blob_count")->evaluate(img, roi, {});
        QCOMPARE(r.measuredValue, 2.0);
        QVERIFY(!r.pass); // default maxBlobs = 0

        r = algo("blob_count")->evaluate(img, roi, { { "maxBlobs", 2.0 } });
        QVERIFY(r.pass);

        // Touching rects merge into one 4-connected component.
        QImage merged = uniformImage(200);
        fillRect(merged, QRect(10, 10, 10, 10), 10);
        fillRect(merged, QRect(20, 10, 10, 10), 10);
        r = algo("blob_count")->evaluate(merged, roi, {});
        QCOMPARE(r.measuredValue, 1.0);
    }

    void edgeStrengthDetectsStructure()
    {
        QImage edge = uniformImage(50);
        fillRect(edge, QRect(50, 0, 50, 100), 200); // vertical step edge
        const QRect roi(0, 0, 100, 100);

        const AlgorithmResult strong = algo("edge_strength")->evaluate(edge, roi, {});
        QVERIFY(strong.pass);
        QVERIFY(strong.measuredValue > 0.0);

        const AlgorithmResult flat = algo("edge_strength")->evaluate(uniformImage(50), roi, {});
        QCOMPARE(flat.measuredValue, 0.0);
        QVERIFY(!flat.pass);
    }

    void engineRunsPlanAndAggregates()
    {
        const QImage img = uniformImage(100);
        InspectionPlan plan;

        const QUuid passing = plan.addRoi(QRectF(10, 10, 30, 30));
        Roi roi = *plan.roiById(passing);
        roi.algorithmId = QStringLiteral("mean_intensity");
        plan.updateRoi(roi);

        plan.addRoi(QRectF(50, 50, 30, 30)); // no algorithm: skipped

        InspectionRunResult result = InspectionEngine::run(img, plan);
        QCOMPARE(int(result.roiResults.size()), 2);
        QCOMPARE(result.evaluatedCount(), 1);
        QCOMPARE(result.passCount(), 1);
        QVERIFY(result.overallPass);
        QVERIFY(result.roiResults[0].evaluated);
        QVERIFY(!result.roiResults[1].evaluated);

        // One failing ROI flips the overall verdict.
        const QUuid failing = plan.addRoi(QRectF(20, 20, 30, 30));
        roi = *plan.roiById(failing);
        roi.algorithmId = QStringLiteral("mean_intensity");
        roi.params = { { "minMean", 150.0 } };
        plan.updateRoi(roi);
        result = InspectionEngine::run(img, plan);
        QVERIFY(!result.overallPass);

        // ROI partially outside the image gets clamped, fully outside is skipped.
        const QUuid outside = plan.addRoi(QRectF(2000, 2000, 50, 50));
        roi = *plan.roiById(outside);
        roi.algorithmId = QStringLiteral("mean_intensity");
        plan.updateRoi(roi);
        result = InspectionEngine::run(img, plan);
        QVERIFY(!result.roiResults[3].evaluated);
    }

    void engineWithNoAssignmentsFailsOverall()
    {
        InspectionPlan plan;
        plan.addRoi(QRectF(0, 0, 10, 10));
        const InspectionRunResult result = InspectionEngine::run(uniformImage(100), plan);
        QVERIFY(!result.overallPass);
        QCOMPARE(result.evaluatedCount(), 0);
    }
};

QTEST_GUILESS_MAIN(TestAlgorithms)
#include "tst_algorithms.moc"
