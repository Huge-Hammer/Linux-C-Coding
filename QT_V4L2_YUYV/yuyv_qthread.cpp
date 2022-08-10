#include "yuyv_qthread.h"

YUYVQThread::YUYVQThread(QWidget *parent):QThread(parent){

    this->parent = (videoshow*)parent;
    show_flag = false;
    v4l2_fd = -1;
    /* 获取输入进行初始化 */
    QStringList device_parm = QCoreApplication::arguments();
    QString str = device_parm[1];
    dev_name = str.toStdString();
}

void YUYVQThread::run(){
    /* 打开设备 */
    v4l2_fd = -1;
    v4l2_fd = open(dev_name.c_str(), O_RDWR | O_NONBLOCK, 0);
    if(v4l2_fd < 0){
        qDebug("open camera failed\n");
        return;
    }
    qDebug("open camera success\n");
    qDebug("------------------------------------\n");

    /* 获取摄像头能力 */
    ioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap);
    /* 判断是否是视频采集设备 */
    if (!(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities)) {
        qDebug("Error:No capture video device!\n");
        return;
    }
    qDebug("Device Information:\n");
    qDebug("driver name: %s\ncard name: %s\n",cap.driver,cap.card);
    qDebug("------------------------------------\n");

    /* 设置采集格式 */
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width  = PIXWIDTH;
    fmt.fmt.pix.height = PIXHEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;   //选择YUYV
    if (0 > ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt)) {
        qDebug("set format failed\n");
        return;
    }
    qDebug("set format success\n");

    /* 判断是否已经设置为我们要求的YUYV像素格式,否则表示该设备不支持YUYV像素格式 */
    if (V4L2_PIX_FMT_YUYV != fmt.fmt.pix.pixelformat) {
        qDebug("Error: the device does not support YUYV format!\n");
        return;
    }
    /* 获取实际的帧宽高度 */
    qDebug("当前视频帧大小<%d * %d>, 颜色空间:%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height,fmt.fmt.pix.colorspace);

    reqbuf.count = FRAMEBUFFER_COUNT;       //帧缓冲的数量
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    if (0 > ioctl(v4l2_fd, VIDIOC_REQBUFS, &reqbuf)) {
        qDebug("request buffer failed\n");
        return;
    }
    qDebug("request buffer success\n");

    /* 建立内存映射 */
    for(n_buffers = 0;n_buffers < FRAMEBUFFER_COUNT;n_buffers++){
        CLEAR(buf);
        buf.index = n_buffers;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if(0 > ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf)){
            qDebug("VIDIOC_QUERYBUF failed\n");
            return;
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ | PROT_WRITE, MAP_SHARED,v4l2_fd, buf.m.offset);
        if (MAP_FAILED == buffers[n_buffers].start) {
            qDebug("mmap error\n");
            return;
        }
    }
    qDebug("memory map success\n");

    /* 入队 */
    for(unsigned int i=0;i<n_buffers;i++){
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (0 > ioctl(v4l2_fd, VIDIOC_QBUF, &buf)) {
            qDebug("入队失败\n");
            return;
        }
    }

    /* 开启视频流 */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 > ioctl(v4l2_fd, VIDIOC_STREAMON, &type)) {
        qDebug("open stream failed\n");
        return;
    }
    qDebug("open stream success\n");

    /* 获取帧数据 */
    while(show_flag){
        do{
            /* 初始化select()来进行I/O端口复用 */
            FD_ZERO(&fds);
            FD_SET(v4l2_fd,&fds);

            /* TimeOut */
            tv.tv_sec  = 2;
            tv.tv_usec = 0;

            ret = select(v4l2_fd+1,&fds,NULL,NULL,&tv);
        } while ((ret == -1) && (errno = EINTR));

        if(ret == -1){
            qDebug("select I/O failed\n");
            return;
        }

        //帧数据处理
        unsigned char onebuf[PIXWIDTH*PIXHEIGHT*3];
        handleData(onebuf);

        try{
            QImage img_stream = QImage(onebuf,PIXWIDTH,PIXHEIGHT,QImage::Format_RGB888);

            if(parent->isVisible()){
                parent->img = img_stream;
                parent->update();
            }

        }catch(...){}
    }
}

YUYVQThread::~YUYVQThread()
{
    exit_show();
}

void YUYVQThread::handleData(unsigned char *bufData){
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    /* 出队 */
    if(0 > ioctl(v4l2_fd,VIDIOC_DQBUF,&buf)){
        qDebug("出队失败\n");
        return;
    }

    //数据处理
    unsigned char temp[buf.bytesused];
    memcpy(temp, buffers[buf.index].start, buf.bytesused);
    yuv_to_rgb(temp, bufData);

    /* 再次入队*/
    if (0 > ioctl(v4l2_fd, VIDIOC_QBUF, &buf)) {
        qDebug("入队失败\n");
        return;
    }
}

void YUYVQThread::exit_show(){
    try{
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (0 > ioctl(v4l2_fd, VIDIOC_STREAMOFF, &type)) {
            return;
        }
        for(int i = 0; i < FRAMEBUFFER_COUNT; i++){
            munmap(buffers[i].start,buffers[i].length);
        }
    }catch(...){}
    qDebug("exit show success\n");
}

void YUYVQThread::yuv_to_rgb(unsigned char *yuv, unsigned char *rgb){
    unsigned int i;
    unsigned char* y0 = yuv + 0;
    unsigned char* u0 = yuv + 1;
    unsigned char* y1 = yuv + 2;
    unsigned char* v0 = yuv + 3;

    unsigned  char* r0 = rgb + 0;
    unsigned  char* g0 = rgb + 1;
    unsigned  char* b0 = rgb + 2;
    unsigned  char* r1 = rgb + 3;
    unsigned  char* g1 = rgb + 4;
    unsigned  char* b1 = rgb + 5;

    float rt0 = 0, gt0 = 0, bt0 = 0, rt1 = 0, gt1 = 0, bt1 = 0;

    for(i = 0; i <= (PIXWIDTH * PIXHEIGHT) / 2 ;i++)
    {
        bt0 = 1.164 * (*y0 - 16) + 2.018 * (*u0 - 128);
        gt0 = 1.164 * (*y0 - 16) - 0.813 * (*v0 - 128) - 0.394 * (*u0 - 128);
        rt0 = 1.164 * (*y0 - 16) + 1.596 * (*v0 - 128);

        bt1 = 1.164 * (*y1 - 16) + 2.018 * (*u0 - 128);
        gt1 = 1.164 * (*y1 - 16) - 0.813 * (*v0 - 128) - 0.394 * (*u0 - 128);
        rt1 = 1.164 * (*y1 - 16) + 1.596 * (*v0 - 128);


        if(rt0 > 250)      rt0 = 255;
        if(rt0< 0)        rt0 = 0;

        if(gt0 > 250)     gt0 = 255;
        if(gt0 < 0)    gt0 = 0;

        if(bt0 > 250)    bt0 = 255;
        if(bt0 < 0)    bt0 = 0;

        if(rt1 > 250)    rt1 = 255;
        if(rt1 < 0)    rt1 = 0;

        if(gt1 > 250)    gt1 = 255;
        if(gt1 < 0)    gt1 = 0;

        if(bt1 > 250)    bt1 = 255;
        if(bt1 < 0)    bt1 = 0;

        *r0 = (unsigned char)rt0;
        *g0 = (unsigned char)gt0;
        *b0 = (unsigned char)bt0;

        *r1 = (unsigned char)rt1;
        *g1 = (unsigned char)gt1;
        *b1 = (unsigned char)bt1;

        yuv = yuv + 4;
        rgb = rgb + 6;
        if(yuv == NULL)
            break;

        y0 = yuv;
        u0 = yuv + 1;
        y1 = yuv + 2;
        v0 = yuv + 3;

        r0 = rgb + 0;
        g0 = rgb + 1;
        b0 = rgb + 2;
        r1 = rgb + 3;
        g1 = rgb + 4;
        b1 = rgb + 5;
    }
}
