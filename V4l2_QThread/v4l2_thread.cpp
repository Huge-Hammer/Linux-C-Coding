#include "v4l2_thread.h"

V4l2Thread::V4l2Thread(QWidget *parent):
    QThread(parent){

    this->parent = (videoshow*)parent;
    show_flag = false;
    v4l2_fd = -1;

    /* 获取输入进行初始化 */
    QStringList device_parm = QCoreApplication::arguments();
    QString str = device_parm[1];
    dev_name = str.toStdString();
}

void V4l2Thread::run(){
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

    /* 查询摄像头所支持的所有像素格式 */
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    qDebug("Support Format:\n");
    CLEAR(cam_fmts);
    while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        // 将枚举出来的格式以及描述信息存放在数组中
        cam_fmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
        memcpy(cam_fmts[fmtdesc.index].description, fmtdesc.description,sizeof(fmtdesc.description));
        fmtdesc.index++;
    }

    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for(int i = 0; cam_fmts[i].pixelformat;i++){
        qDebug("format<0x%x>, description<%s>\n", cam_fmts[i].pixelformat, cam_fmts[i].description);

        /* 枚举出摄像头所支持的所有视频采集分辨率 */
        frmsize.index = 0;
        frmsize.pixel_format = cam_fmts[i].pixelformat;
        frmival.pixel_format = cam_fmts[i].pixelformat;
        while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {

            frmsize.index++;

            /* 获取摄像头视频采集帧率 */
            frmival.index = 0;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)) {

                qDebug("size<%d*%d> <%dfps>", frmsize.discrete.width,frmsize.discrete.height,frmival.discrete.denominator / frmival.discrete.numerator);
                frmival.index++;
            }
        }
    }
    qDebug("------------------------------------\n");

    /* 设置采集格式 */
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width  = PIXWIDTH;
    fmt.fmt.pix.height = PIXHEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;   //选择MJPEG
    if (0 > ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt)) {
        qDebug("set format failed\n");
        return;
    }
    qDebug("set format success\n");

    /* 判断是否已经设置为我们要求的MJPEG像素格式,否则表示该设备不支持MJPEG像素格式 */
    if (V4L2_PIX_FMT_MJPEG != fmt.fmt.pix.pixelformat) {
        qDebug("Error: the device does not support MJPEG format!\n");
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

    /* calloc函数为buffer_infos动态分配内存空间并初始化为0*/
    buffer_infos = (struct buffer_info*)calloc(FRAMEBUFFER_COUNT,sizeof(struct buffer_info));
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

        buffer_infos[n_buffers].length = buf.length;
        buffer_infos[n_buffers].start  = mmap(NULL, buf.length,PROT_READ | PROT_WRITE, MAP_SHARED,v4l2_fd, buf.m.offset);

        if (MAP_FAILED == buffer_infos[n_buffers].start) {
            qDebug("mmap error\n");
            return;
        }
    }
    qDebug("memory map success\n");

    /* 入队 */
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {
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

        CLEAR(one_buf);
        one_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        one_buf.memory = V4L2_MEMORY_MMAP;

        /* 出队 */
        if(0 > ioctl(v4l2_fd,VIDIOC_DQBUF,&one_buf)){
            qDebug("出队失败\n");
            return;
        }

        try{
            FrameBuffer frame;
            v4l2_get_one_frame(&frame);

            QImage img_stream;
            if(parent->isVisible()){
                img_stream.loadFromData(frame.buf,frame.length);
                parent->img = img_stream;
                parent->update();
            }

        }catch(...){}

        /* 再次入队*/
        if (0 > ioctl(v4l2_fd, VIDIOC_QBUF, &one_buf)) {
            qDebug("入队失败\n");
            return;
        }

    }
    qDebug("while done!\n");
}

V4l2Thread::~V4l2Thread(){

}

void V4l2Thread::exit_show(){
    try{
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (0 > ioctl(v4l2_fd, VIDIOC_STREAMOFF, &type)) {
            qDebug("close stream failed\n");
        }
        for(int i = 0; i < FRAMEBUFFER_COUNT; i++){
            munmap(buffer_infos[i].start,buffer_infos[i].length);
        }
    }catch(...){}
    qDebug("exit show success\n");
}

void V4l2Thread::v4l2_get_one_frame(FrameBuffer *framebuf){
    memcpy(framebuf->buf,(char *)buffer_infos[one_buf.index].start,one_buf.bytesused);//bytesused 表示buf中已经使用的字节数
    framebuf->length = one_buf.bytesused;
}
