#include "MainWindow.h"

#include "pages/AlgorithmsPage.h"
#include "pages/ScanPage.h"
#include "pages/SetupPage.h"
#include "pages/ViewerPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QStatusBar>

namespace xri {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("XRI Demo — X-Ray Inspection"));

    m_sidebar = new QListWidget;
    m_sidebar->setObjectName("sidebarList");
    m_sidebar->setFixedWidth(180);
    m_sidebar->addItem(tr("1. Scan"));
    m_sidebar->addItem(tr("2. Viewer"));
    m_sidebar->addItem(tr("3. Inspection Setup"));
    m_sidebar->addItem(tr("4. Algorithms"));

    m_scanPage = new ScanPage(&m_session);
    auto* viewerPage = new ViewerPage(&m_session);
    auto* setupPage = new SetupPage(&m_session);
    auto* algorithmsPage = new AlgorithmsPage(&m_session);

    m_stack = new QStackedWidget;
    m_stack->setObjectName("pagesStack");
    m_stack->addWidget(m_scanPage);
    m_stack->addWidget(viewerPage);
    m_stack->addWidget(setupPage);
    m_stack->addWidget(algorithmsPage);

    auto* central = new QWidget;
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_sidebar);
    layout->addWidget(m_stack, 1);
    setCentralWidget(central);

    m_stateLabel = new QLabel;
    m_stateLabel->setObjectName("stateLabel");
    statusBar()->addPermanentWidget(m_stateLabel);

    connect(m_sidebar, &QListWidget::currentRowChanged, m_stack,
            &QStackedWidget::setCurrentIndex);
    m_sidebar->setCurrentRow(PageScan);

    for (QWidget* page : { static_cast<QWidget*>(m_scanPage), static_cast<QWidget*>(viewerPage),
                           static_cast<QWidget*>(setupPage),
                           static_cast<QWidget*>(algorithmsPage) }) {
        connect(page, SIGNAL(statusMessage(QString)), statusBar(),
                SLOT(showMessage(QString)));
    }

    connect(m_scanPage, &ScanPage::scanCompleted, this, [this] { switchToPage(PageViewer); });

    connect(&m_session, &InspectionSession::scanChanged, this, &MainWindow::updateStateLabel);
    connect(&m_session, &InspectionSession::resultsChanged, this, &MainWindow::updateStateLabel);
    connect(m_session.plan(), &InspectionPlan::roiAdded, this, &MainWindow::updateStateLabel);
    connect(m_session.plan(), &InspectionPlan::roiRemoved, this, &MainWindow::updateStateLabel);
    connect(m_session.plan(), &InspectionPlan::roiChanged, this, &MainWindow::updateStateLabel);
    connect(m_session.plan(), &InspectionPlan::planReset, this, &MainWindow::updateStateLabel);
    updateStateLabel();
}

void MainWindow::switchToPage(int index)
{
    m_sidebar->setCurrentRow(index);
}

int MainWindow::currentPageIndex() const
{
    return m_stack->currentIndex();
}

void MainWindow::updateStateLabel()
{
    QStringList parts;

    if (m_session.hasScan()) {
        parts << tr("Scan %1×%2")
                     .arg(m_session.scanImage().width())
                     .arg(m_session.scanImage().height());
    } else {
        parts << tr("No scan");
    }

    int assigned = 0;
    for (const Roi& roi : m_session.plan()->rois())
        assigned += roi.algorithmId.isEmpty() ? 0 : 1;
    parts << tr("%1 ROIs (%2 assigned)").arg(m_session.plan()->count()).arg(assigned);

    if (m_session.hasResult()) {
        const InspectionRunResult& result = m_session.lastResult();
        parts << tr("Last run: %1 (%2/%3 passed)")
                     .arg(result.overallPass ? tr("PASS") : tr("FAIL"))
                     .arg(result.passCount())
                     .arg(result.evaluatedCount());
    }

    m_stateLabel->setText(parts.join(QStringLiteral("  |  ")));
}

} // namespace xri
