#ifndef V4L2_THREAD_H
#define V4L2_THREAD_H

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

#include "videoshow.h"

#define FRAMEBUFFER_COUNT 4
#define PIXWIDTH    640
#define PIXHEIGHT   480

#define CLEAR(x) memset(&(x), 0, sizeof(x))

/* 命名空间 */
using namespace std;

class V4l2Thread : public QThread{
public:
    explicit V4l2Thread(QWidget *parent = 0);
    ~V4l2Thread();

    /* 视频显示标志位信号 */
    bool show_flag;

    videoshow *parent;

    /* v4l2结构体 */
    struct v4l2_format              fmt;
    struct v4l2_buffer              buf,one_buf;
    struct v4l2_requestbuffers      reqbuf;
    struct v4l2_capability          cap;
    struct v4l2_fmtdesc             fmtdesc;
    struct v4l2_frmsizeenum         frmsize;
    struct v4l2_frmivalenum         frmival;
    enum v4l2_buf_type              type;
    fd_set                          fds;
    struct timeval                  tv;
    int                             ret, v4l2_fd;    //返回值ret和v4l2描述符v4l2_fd
    unsigned int                    n_buffers;
    string                          dev_name;

    /* buffer描述信息结构体 */
    struct buffer_info {
        void *start;
        unsigned int length;
    };

    /* 摄像头像素格式及其描述信息结构体 */
    typedef struct camera_format {
        unsigned char description[32];  //字符串描述信息
        unsigned int pixelformat;       //像素格式
    } cam_fmt;

    /* 帧描述信息结构体 */
    typedef struct Frame_Buffer{
        unsigned char buf[PIXHEIGHT*PIXWIDTH*3];
        int length;
    }FrameBuffer;


    struct buffer_info *buffer_infos;

    cam_fmt cam_fmts[10];

    /* QThread 虚函数run */
    void run();

    void exit_show();

    void v4l2_get_one_frame(FrameBuffer * framebuf);

};

#endif // V4L2_THREAD_H
