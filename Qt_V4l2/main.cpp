#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    if (2 != argc) {
        fprintf(stderr, "Usage: %s <video_dev>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
