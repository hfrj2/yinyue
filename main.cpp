#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("朝琴音乐播放器");
    app.setOrganizationName("朝琴工作室");

    MainWindow window;
    window.show();

    return app.exec();
}
