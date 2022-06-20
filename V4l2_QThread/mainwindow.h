#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>
#include <QWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGuiApplication>
#include <QScreen>
#include <QCoreApplication>
#include <QDir>

#include "videoshow.h"
#include "v4l2_thread.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    /* 主容器，Widget也可以当作一种容器 */
    QWidget *mainWidget;
    QWidget *rightWidget;
    /* 界面水平区域布局 */
    QHBoxLayout *hboxLayout;
    /* 界面垂直区域布局 */
    QVBoxLayout *vboxLayout;
    videoshow   *video;
    QPushButton *startCapture;
    QPushButton *takePic;
    QPushButton *exitBtn;

    V4l2Thread *t;

    /* 保存照片的序号 */
    int pic_num;
    /* 照片保存位置 */
    QDir *dir;

private slots:
    void onExitClicked();
    void onStartCapture();
    void onTakePic();
};

#endif // MAINWINDOW_H
