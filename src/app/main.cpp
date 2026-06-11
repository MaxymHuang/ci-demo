#include "MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QStyleFactory>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("XRI Demo"));
    app.setOrganizationName(QStringLiteral("CI Demo"));
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QFile qss(QStringLiteral(":/resources/dark.qss"));
    if (qss.open(QIODevice::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    xri::MainWindow window;
    window.resize(1200, 800);
    window.show();
    return app.exec();
}
