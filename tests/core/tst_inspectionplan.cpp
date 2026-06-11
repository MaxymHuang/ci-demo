#include "core/InspectionPlan.h"
#include "core/RoiListModel.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>

using namespace xri;

class TestInspectionPlan : public QObject
{
    Q_OBJECT
private slots:
    void addRemoveUpdate()
    {
        InspectionPlan plan;
        QSignalSpy addedSpy(&plan, &InspectionPlan::roiAdded);
        QSignalSpy removedSpy(&plan, &InspectionPlan::roiRemoved);
        QSignalSpy changedSpy(&plan, &InspectionPlan::roiChanged);

        const QUuid first = plan.addRoi(QRectF(10, 10, 50, 40));
        const QUuid second = plan.addRoi(QRectF(100, 20, 30, 30));
        QCOMPARE(plan.count(), 2);
        QCOMPARE(addedSpy.count(), 2);
        QCOMPARE(plan.roiAt(0)->name, QStringLiteral("ROI 1"));
        QCOMPARE(plan.roiAt(1)->name, QStringLiteral("ROI 2"));

        Roi updated = *plan.roiById(first);
        updated.algorithmId = QStringLiteral("mean_intensity");
        QVERIFY(plan.updateRoi(updated));
        QCOMPARE(changedSpy.count(), 1);
        QCOMPARE(plan.roiById(first)->algorithmId, QStringLiteral("mean_intensity"));

        QVERIFY(plan.removeRoi(second));
        QCOMPARE(plan.count(), 1);
        QCOMPARE(removedSpy.count(), 1);
        QVERIFY(!plan.removeRoi(second)); // already gone
    }

    void updateUnknownIdFails()
    {
        InspectionPlan plan;
        Roi roi;
        roi.rect = QRectF(0, 0, 10, 10);
        QVERIFY(!plan.updateRoi(roi));
    }

    void normalizesRectOnAdd()
    {
        InspectionPlan plan;
        plan.addRoi(QRectF(QPointF(60, 50), QPointF(10, 10))); // inverted drag
        QCOMPARE(plan.roiAt(0)->rect, QRectF(10, 10, 50, 40));
    }

    void jsonRoundTrip()
    {
        InspectionPlan plan;
        const QUuid id = plan.addRoi(QRectF(5, 6, 70, 80));
        Roi roi = *plan.roiById(id);
        roi.algorithmId = QStringLiteral("blob_count");
        roi.params = { { "threshold", 42.0 } };
        plan.updateRoi(roi);
        plan.addRoi(QRectF(1, 2, 3, 4));

        InspectionPlan restored;
        QVERIFY(restored.fromJson(plan.toJson()));
        QCOMPARE(restored.count(), plan.count());
        QCOMPARE(restored.rois()[0], plan.rois()[0]);
        QCOMPARE(restored.rois()[1], plan.rois()[1]);

        // Name counter restored: the next ROI must not reuse "ROI 2".
        restored.addRoi(QRectF(0, 0, 9, 9));
        QCOMPARE(restored.roiAt(2)->name, QStringLiteral("ROI 3"));
    }

    void listModelFollowsPlan()
    {
        InspectionPlan plan;
        RoiListModel model(&plan);
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        plan.addRoi(QRectF(10, 10, 20, 20));
        plan.addRoi(QRectF(40, 40, 20, 20));
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0), Qt::DisplayRole).toString(), QStringLiteral("ROI 1"));
        QCOMPARE(model.data(model.index(1), RoiListModel::RectRole).toRectF(),
                 QRectF(40, 40, 20, 20));

        QVERIFY(model.setData(model.index(0), QStringLiteral("Solder joint"), Qt::EditRole));
        QCOMPARE(plan.roiAt(0)->name, QStringLiteral("Solder joint"));
        QVERIFY(!model.setData(model.index(0), QStringLiteral("  "), Qt::EditRole));

        plan.removeRoi(plan.roiAt(0)->id);
        QCOMPARE(model.rowCount(), 1);

        plan.clear();
        QCOMPARE(model.rowCount(), 0);
    }
};

QTEST_GUILESS_MAIN(TestInspectionPlan)
#include "tst_inspectionplan.moc"
