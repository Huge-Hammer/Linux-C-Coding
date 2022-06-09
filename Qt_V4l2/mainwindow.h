#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QDebug>
#include <QTimer>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <stdbool.h>
#include <string>

/* 命名空间 */
using namespace std;

#define FRAMEBUFFER_COUNT 4
#define PIXWIDTH    640
#define PIXHEIGHT   480

/* buffer描述信息 */
struct buffer_info {
    void *start;
    unsigned int length;
};

/* 摄像头像素格式及其描述信息 */
typedef struct camera_format {
    unsigned char description[32];  //字符串描述信息
    unsigned int pixelformat;       //像素格式
} cam_fmt;

/* 帧描述信息 */
typedef struct Frame_Buffer{
    unsigned char buf[PIXHEIGHT*PIXWIDTH*3];
    int length;
}FrameBuffer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

    /* v4l2函数 */
    int v4l2_dev_init(string device_name);

    int v4l2_set_format();

    int v4l2_init_buffer();

    int v4l2_stream_on();

    int v4l2_get_one_frame(FrameBuffer * framebuf);

//    int v4l2_device_release();
    ~MainWindow();

private:
    /* 主容器，Widget也可以当作一种容器 */
    QWidget *mainWidget;
    /* 界面右侧区域容器 */
    QWidget *rightWidget;
    /* 滚动区域，方便开发高分辨率 */
    QScrollArea *scrollArea;
    /* 将采集到的图像使用Widget显示 */
    QLabel *displayLabel;
    /* 界面水平区域布局 */
    QHBoxLayout *hboxLayout;
    /* 界面垂直区域布局 */
    QVBoxLayout *vboxLayout;
    /* 开启/关闭摄像头按钮 */
    QPushButton *pushButton[2];

    /* 布局初始化 */
    void layoutInit();

    /* 摄像头描述符 */
    int v4l2_fd;
    /* 定时器 */
    QTimer *timer;
    /* buffer */
    struct buffer_info *buffer_infos;

    cam_fmt cam_fmts[10];

private slots:
    /* 视频显示 */
    void video_show();
    /* 定时器启动*/
    void timer_start();
    int v4l2_device_release();
};

#endif // MAINWINDOW_H
