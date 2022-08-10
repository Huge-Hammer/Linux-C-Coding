#ifndef YUYV_QTHREAD_H
#define YUYV_QTHREAD_H

#include <QThread>
#include <QDebug>
#include <QCoreApplication>
#include <QMutex>

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

#include "video_show.h"

#define FRAMEBUFFER_COUNT 4
#define PIXWIDTH    640
#define PIXHEIGHT   480

#define CLEAR(x) memset(&(x), 0, sizeof(x))
/* 命名空间 */
using namespace std;

class YUYVQThread : public QThread{
public:
    explicit YUYVQThread(QWidget *parent = 0);
    ~YUYVQThread();

    /* 视频显示标志位信号 */
    bool show_flag;

    videoshow *parent;

    /* v4l2结构体 */
    struct v4l2_format              fmt;
    struct v4l2_buffer              buf;
    struct v4l2_requestbuffers      reqbuf;
    struct v4l2_capability          cap;
    enum v4l2_buf_type              type;
    fd_set                          fds;
    struct timeval                  tv;
    int                             ret, v4l2_fd;    //返回值ret和v4l2描述符v4l2_fd
    unsigned int                    n_buffers;
    string                          dev_name;

    /* buffer描述信息结构体 */
    struct buffer {
        void *start;
        size_t length;
    };

    struct buffer buffers[FRAMEBUFFER_COUNT];

    /* QThread 虚函数run */
    void run();

    void exit_show();
    void handleData(unsigned char *bufData);
    void yuv_to_rgb(unsigned char* yuv, unsigned char *rgb);
};
#endif // YUYV_QTHREAD_H
