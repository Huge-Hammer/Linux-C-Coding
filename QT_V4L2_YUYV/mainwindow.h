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
#include <QDateTime>
#include <QCloseEvent>

#include "video_show.h"
#include "yuyv_qthread.h"

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
    QPushButton *startBtn;
    QPushButton *capBtn;
    QPushButton *recBtn;

    YUYVQThread *t;

    /* 照片保存位置 */
    QDir *dir;

private slots:
    void onRec();
    void onStart();
    void onCap();

    //重写窗口关闭事件
protected :
    void closeEvent(QCloseEvent *event);
};

#endif // MAINWINDOW_H
