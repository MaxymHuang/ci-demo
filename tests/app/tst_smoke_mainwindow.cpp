#include "MainWindow.h"
#include "pages/ScanPage.h"
#include "widgets/RoiEditorView.h"

#include <QComboBox>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTest>
#include <QToolButton>

using namespace xri;

// Drives the real MainWindow on the offscreen platform: navigation, a full
// mock scan, mouse-drawn ROI creation, algorithm assignment and an
// inspection run. Proves the complex-UI-in-headless-CI claim end to end.
class TestSmokeMainWindow : public QObject
{
    Q_OBJECT
private:
    static void dragOnViewport(QWidget* viewport, const QPoint& from, const QPoint& to)
    {
        QTest::mousePress(viewport, Qt::LeftButton, {}, from);
        // QTest::mouseMove does not synthesize button-down move events, so
        // deliver one directly.
        QMouseEvent move(QEvent::MouseMove, to, viewport->mapToGlobal(to), Qt::NoButton,
                         Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(viewport, &move);
        QTest::mouseRelease(viewport, Qt::LeftButton, {}, to);
    }

private slots:
    void fullWorkflow()
    {
        MainWindow window;
        window.resize(1200, 800);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        auto* sidebar = window.findChild<QListWidget*>("sidebarList");
        auto* stack = window.findChild<QStackedWidget*>("pagesStack");
        QVERIFY(sidebar);
        QVERIFY(stack);
        QCOMPARE(stack->count(), 4);

        // --- Sidebar navigation drives the page stack ---
        for (int row = 0; row < sidebar->count(); ++row) {
            const QPoint center = sidebar->visualItemRect(sidebar->item(row)).center();
            QTest::mouseClick(sidebar->viewport(), Qt::LeftButton, {}, center);
            QCOMPARE(stack->currentIndex(), row);
        }

        // --- Scan: instant ticks, then a finished image lands in the session ---
        window.switchToPage(MainWindow::PageScan);
        window.scanPage()->simulator()->setTickInterval(0);

        auto* startButton = window.findChild<QPushButton*>("startScanButton");
        QVERIFY(startButton);
        QSignalSpy scanSpy(window.session(), &InspectionSession::scanChanged);
        QTest::mouseClick(startButton, Qt::LeftButton);
        QTRY_COMPARE(scanSpy.count(), 1);
        QVERIFY(window.session()->hasScan());
        QCOMPARE(window.session()->scanImage().size(), QSize(640, 480));

        // Finishing a scan auto-navigates to the viewer.
        QCOMPARE(stack->currentIndex(), int(MainWindow::PageViewer));

        // --- Setup: draw an ROI with a mouse drag ---
        window.switchToPage(MainWindow::PageSetup);
        auto* drawButton = window.findChild<QToolButton*>("drawModeButton");
        auto* editor = window.findChild<RoiEditorView*>("roiEditorView");
        QVERIFY(drawButton);
        QVERIFY(editor);
        QTest::mouseClick(drawButton, Qt::LeftButton);

        dragOnViewport(editor->viewport(), QPoint(50, 50), QPoint(220, 180));
        QTRY_COMPARE(window.session()->plan()->count(), 1);
        const Roi roi = window.session()->plan()->rois().front();
        QVERIFY(roi.rect.width() >= 8.0);
        QVERIFY(roi.rect.height() >= 8.0);

        // --- Algorithms: assign via combo box, run, verify verdict ---
        window.switchToPage(MainWindow::PageAlgorithms);
        auto* roiList = window.findChild<QListView*>("algoRoiList");
        auto* combo = window.findChild<QComboBox*>("algorithmCombo");
        auto* runButton = window.findChild<QPushButton*>("runInspectionButton");
        auto* overallLabel = window.findChild<QLabel*>("overallResultLabel");
        auto* exportButton = window.findChild<QPushButton*>("exportButton");
        QVERIFY(roiList);
        QVERIFY(combo);
        QVERIFY(runButton);
        QVERIFY(overallLabel);
        QVERIFY(exportButton);

        roiList->setCurrentIndex(roiList->model()->index(0, 0));
        QVERIFY(combo->isEnabled());
        combo->setCurrentIndex(combo->findData(QStringLiteral("mean_intensity")));
        QCOMPARE(window.session()->plan()->roiAt(0)->algorithmId,
                 QStringLiteral("mean_intensity"));

        QVERIFY(!exportButton->isEnabled());
        QSignalSpy resultsSpy(window.session(), &InspectionSession::resultsChanged);
        QTest::mouseClick(runButton, Qt::LeftButton);
        QTRY_VERIFY(resultsSpy.count() >= 1);
        QVERIFY(window.session()->hasResult());
        QCOMPARE(window.session()->lastResult().evaluatedCount(), 1);
        QVERIFY(overallLabel->text().contains(QStringLiteral("PASS"))
                || overallLabel->text().contains(QStringLiteral("FAIL")));
        QVERIFY(exportButton->isEnabled());
    }
};

QTEST_MAIN(TestSmokeMainWindow)
#include "tst_smoke_mainwindow.moc"
