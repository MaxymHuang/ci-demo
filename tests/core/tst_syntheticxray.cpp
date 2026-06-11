#include "core/SyntheticXraySource.h"

#include <QTest>

using namespace xri;

class TestSyntheticXray : public QObject
{
    Q_OBJECT
private slots:
    void sizeAndFormat()
    {
        ScanParams p;
        p.width = 320;
        p.height = 200;
        const QImage img = SyntheticXraySource::generate(p);
        QCOMPARE(img.size(), QSize(320, 200));
        QCOMPARE(img.format(), QImage::Format_Grayscale8);
    }

    void deterministicForSameSeed()
    {
        ScanParams p;
        p.seed = 1234;
        QCOMPARE(SyntheticXraySource::generate(p), SyntheticXraySource::generate(p));
    }

    void differentSeedsDiffer()
    {
        ScanParams a;
        a.seed = 1;
        ScanParams b = a;
        b.seed = 2;
        QVERIFY(SyntheticXraySource::generate(a) != SyntheticXraySource::generate(b));
    }

    void meanIntensityPlausible()
    {
        const QImage img = SyntheticXraySource::generate(ScanParams{});
        qint64 sum = 0;
        for (int y = 0; y < img.height(); ++y) {
            const uchar* line = img.constScanLine(y);
            for (int x = 0; x < img.width(); ++x)
                sum += line[x];
        }
        const double mean = double(sum) / (qint64(img.width()) * img.height());
        QVERIFY2(mean > 100.0 && mean < 230.0, qPrintable(QString::number(mean)));
    }

    void defectDarkensImage()
    {
        ScanParams with;
        with.addDefect = true;
        ScanParams without = with;
        without.addDefect = false;

        const auto minPixel = [](const QImage& img) {
            int minValue = 255;
            for (int y = 0; y < img.height(); ++y) {
                const uchar* line = img.constScanLine(y);
                for (int x = 0; x < img.width(); ++x)
                    minValue = std::min(minValue, int(line[x]));
            }
            return minValue;
        };

        // The dense inclusion must produce clearly darker pixels than any
        // regular object does.
        QVERIFY(minPixel(SyntheticXraySource::generate(with))
                < minPixel(SyntheticXraySource::generate(without)) - 15);
    }
};

QTEST_GUILESS_MAIN(TestSyntheticXray)
#include "tst_syntheticxray.moc"
