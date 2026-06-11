#include "core/ScanSimulator.h"

#include <QSignalSpy>
#include <QTest>

using namespace xri;

class TestScanSimulator : public QObject
{
    Q_OBJECT
private slots:
    void emitsMonotonicProgressAndFinishes()
    {
        ScanSimulator simulator;
        simulator.setTickInterval(0);

        QSignalSpy progressSpy(&simulator, &ScanSimulator::progressChanged);
        QSignalSpy finishedSpy(&simulator, &ScanSimulator::finished);

        ScanParams params;
        params.width = 64;
        params.height = 48;
        simulator.start(params);

        QTRY_COMPARE(finishedSpy.count(), 1);
        QVERIFY(!simulator.isRunning());

        QVERIFY(progressSpy.count() >= 2);
        int last = -1;
        for (const QList<QVariant>& args : progressSpy) {
            const int percent = args.first().toInt();
            QVERIFY(percent >= last);
            last = percent;
        }
        QCOMPARE(last, 100);
    }

    void finishedImageMatchesDirectGeneration()
    {
        ScanSimulator simulator;
        simulator.setTickInterval(0);
        QSignalSpy finishedSpy(&simulator, &ScanSimulator::finished);

        ScanParams params;
        params.width = 64;
        params.height = 48;
        params.seed = 99;
        simulator.start(params);

        QTRY_COMPARE(finishedSpy.count(), 1);
        const QImage finished = finishedSpy.first().first().value<QImage>();
        QCOMPARE(finished, SyntheticXraySource::generate(params));
    }

    void cancelStopsTheScan()
    {
        ScanSimulator simulator;
        simulator.setTickInterval(1000); // never ticks within the test
        QSignalSpy cancelledSpy(&simulator, &ScanSimulator::cancelled);
        QSignalSpy finishedSpy(&simulator, &ScanSimulator::finished);

        simulator.start(ScanParams{});
        QVERIFY(simulator.isRunning());
        simulator.cancel();

        QVERIFY(!simulator.isRunning());
        QCOMPARE(cancelledSpy.count(), 1);
        QCOMPARE(finishedSpy.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestScanSimulator)
#include "tst_scansimulator.moc"
