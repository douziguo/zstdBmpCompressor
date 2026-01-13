//
// Created by douziguo on 2026/1/13.
//

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("ZstdBmpCompress Demo");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("TestDemo");

    MainWindow window;
    window.show();

    return app.exec();
}