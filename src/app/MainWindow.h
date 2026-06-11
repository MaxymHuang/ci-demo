#pragma once

#include "core/InspectionSession.h"

#include <QMainWindow>

class QLabel;
class QListWidget;
class QStackedWidget;

namespace xri {

class ScanPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    InspectionSession* session() { return &m_session; }
    ScanPage* scanPage() { return m_scanPage; }

    enum Page { PageScan = 0, PageViewer, PageSetup, PageAlgorithms };
    void switchToPage(int index);
    int currentPageIndex() const;

private:
    void updateStateLabel();

    InspectionSession m_session;
    QListWidget* m_sidebar;
    QStackedWidget* m_stack;
    QLabel* m_stateLabel;
    ScanPage* m_scanPage;
};

} // namespace xri
