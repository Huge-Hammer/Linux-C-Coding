#include "mainwindow.h"
#include <QGuiApplication>
#include <QScreen>
#include <QFile>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    /* 布局初始化 */
    layoutInit();

    /* 获取输入进行初始化 */
    QStringList device_parm = QCoreApplication::arguments();
    QString str = device_parm[1];

    v4l2_dev_init(str.toStdString());
    v4l2_stream_on();

    //点击开始按钮，打开定时器
    connect(pushButton[0],SIGNAL(clicked()),this,SLOT(timer_start()));

    //每隔固定的时间显示一帧
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(video_show()));

    /* 按钮窗口关闭，先释放设备，再关闭窗口 */
    connect(pushButton[1],SIGNAL(clicked()),this,SLOT(v4l2_device_release()));
    connect(pushButton[1],SIGNAL(clicked()),this,SLOT(close()));
}

MainWindow::~MainWindow()
{

}

void MainWindow::layoutInit(){
    /* 获取屏幕的分辨率，Qt官方建议使用这种方法获取屏幕分辨率，防上多屏设备导致对应不上 */
    QList <QScreen *> list_screen =  QGuiApplication::screens();

    /* 如果是ARM平台，直接设置大小为屏幕的大小 */
#if __arm__
    /* 重设大小 */
    this->resize(list_screen.at(0)->geometry().width(),
                 list_screen.at(0)->geometry().height());
#else
    /* 否则则设置主窗体大小为800x480 */
    this->resize(800, 480);
#endif

    /* 实例化与布局，常规操作 */
    mainWidget = new QWidget();
    rightWidget = new QWidget();
    pushButton[0] = new QPushButton();
    pushButton[1] = new QPushButton();
    scrollArea = new QScrollArea();
    displayLabel = new QLabel(scrollArea);
    vboxLayout = new QVBoxLayout();
    hboxLayout = new QHBoxLayout();

    vboxLayout->addWidget(pushButton[0]);
    vboxLayout->addWidget(pushButton[1]);
    rightWidget->setLayout(vboxLayout);
    hboxLayout->addWidget(scrollArea);
    hboxLayout->addWidget(rightWidget);
    mainWidget->setLayout(hboxLayout);
    this->setCentralWidget(mainWidget);

    pushButton[0]->setMaximumHeight(40);
    pushButton[0]->setMaximumWidth(200);
    pushButton[1]->setMaximumHeight(40);
    pushButton[1]->setMaximumWidth(200);
    scrollArea->setMinimumWidth(this->width() - pushButton[0]->width());
    displayLabel->setMinimumWidth(scrollArea->width() * 0.75);
    displayLabel->setMinimumHeight(scrollArea->height() * 0.75);
    scrollArea->setWidget(displayLabel);
    /* 居中显示 */
    scrollArea->setAlignment(Qt::AlignCenter);
    /* 自动拉伸 */
    displayLabel->setScaledContents(true);
    pushButton[0]->setText("开始");
    pushButton[1]->setText("结束");
}

void MainWindow::video_show(){
     FrameBuffer frame;
     QPixmap pix;

     //获取一帧显示
     v4l2_get_one_frame(&frame);
     pix.loadFromData(frame.buf, frame.length);
     pix.scaled(displayLabel->width(),displayLabel->height(),Qt::KeepAspectRatio);
     displayLabel->setPixmap(pix);

}

/* 定时器控制帧 */
void MainWindow::timer_start(){
    // 1000/33约等于30,也就是每一秒显示30帧
    timer->start(33);
}

int MainWindow::v4l2_dev_init(string device_name){
    /* 打开摄像头 */
    v4l2_fd = open(device_name.c_str(),O_RDWR);
    if(v4l2_fd < 0){
        printf("open camera failed\n");
        return -1;
    }
    printf("open camera success\n");
    printf("------------------------------------\n");

    /* 获取摄像头能力 */
    struct v4l2_capability cap;
    // 查询设备功能
    ioctl(v4l2_fd, VIDIOC_QUERYCAP, &cap);
    // 判断是否是视频采集设备
    if (!(V4L2_CAP_VIDEO_CAPTURE & cap.capabilities)) {
        printf("Error:No capture video device!\n");
        return -1;
    }
    printf("Device Information:\n");
    printf("driver name: %s\ncard name: %s\n",cap.driver,cap.card);
    printf("------------------------------------\n");

    /* 查询摄像头所支持的所有像素格式 */
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support Format:\n");
    /* 先将数组清空 */
    memset(&cam_fmts,0,sizeof(cam_fmts));
    while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        // 将枚举出来的格式以及描述信息存放在数组中
        cam_fmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
        memcpy(cam_fmts[fmtdesc.index].description, fmtdesc.description,sizeof(fmtdesc.description));
        fmtdesc.index++;
    }

    /* 打印格式 */
    struct v4l2_frmsizeenum frmsize;
    struct v4l2_frmivalenum frmival;
    int i;

    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for(i = 0; cam_fmts[i].pixelformat;i++){
        printf("format<0x%x>, description<%s>\n", cam_fmts[i].pixelformat, cam_fmts[i].description);

        /* 枚举出摄像头所支持的所有视频采集分辨率 */
        frmsize.index = 0;
        frmsize.pixel_format = cam_fmts[i].pixelformat;
        frmival.pixel_format = cam_fmts[i].pixelformat;
        while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) {

            printf("size<%d*%d> ",frmsize.discrete.width,frmsize.discrete.height);
            frmsize.index++;

            /* 获取摄像头视频采集帧率 */
            frmival.index = 0;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            while (0 == ioctl(v4l2_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)) {

                printf("<%dfps>", frmival.discrete.denominator / frmival.discrete.numerator);
                frmival.index++;
            }
            printf("\n");
        }
        printf("\n");
    }
    printf("-------------------------------------\n");

    /* 调用函数，设置摄像头格式 */
    if(v4l2_set_format()<0){
        printf("set format failed\n");
        return -1;
    }

    return 0;
}

int MainWindow::v4l2_set_format(){
    struct v4l2_format fmt;
    struct v4l2_streamparm streamparm;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width  = PIXWIDTH;
    fmt.fmt.pix.height = PIXHEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;   //选择MJPEG
    if (0 > ioctl(v4l2_fd, VIDIOC_S_FMT, &fmt)) {
        printf("set format failed\n");
        return -1;
    }
    printf("set format success\n");

    /* 判断是否已经设置为我们要求的MJPEG像素格式,否则表示该设备不支持MJPEG像素格式 */
    if (V4L2_PIX_FMT_MJPEG != fmt.fmt.pix.pixelformat) {
        printf("Error: the device does not support MJPEG format!\n");
        return -1;
    }
    /* 获取实际的帧宽高度 */
    printf("当前视频帧大小<%d * %d>, 颜色空间:%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height,fmt.fmt.pix.colorspace);

    /* 获取streamparm */
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 > ioctl(v4l2_fd, VIDIOC_G_PARM, &streamparm)) {
        printf("get parm failed\n");
        return -1;
    }

    /* 判断是否支持帧率设置 */
    if (V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability) {
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 30;//30fps
        if (0 > ioctl(v4l2_fd, VIDIOC_S_PARM, &streamparm)) {
            printf("Error:device do not support set fps\n");
            return -1;
        }
    }

    return 0;
}

int MainWindow::v4l2_init_buffer(){
    struct v4l2_requestbuffers reqbuf;
    reqbuf.count = FRAMEBUFFER_COUNT;       //帧缓冲的数量
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    if (0 > ioctl(v4l2_fd, VIDIOC_REQBUFS, &reqbuf)) {
        printf("request buffer failed\n");
        return -1;
    }
    printf("request buffer success\n");

    /* 建立内存映射 */
    struct v4l2_buffer buf;
    unsigned int n_buffers = 0;
    /* calloc函数为buffer_infos动态分配内存空间并初始化为0*/
    buffer_infos = (struct buffer_info*)calloc(FRAMEBUFFER_COUNT,sizeof(struct buffer_info));

    for (n_buffers = 0; n_buffers < FRAMEBUFFER_COUNT; n_buffers++) {
        memset(&buf,0,sizeof(buf));
        buf.index = n_buffers;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if(0 > ioctl(v4l2_fd, VIDIOC_QUERYBUF, &buf)){
            printf("VIDIOC_QUERYBUF failed\n");
            return -1;
        };

        buffer_infos[n_buffers].start = mmap(NULL, buf.length,PROT_READ | PROT_WRITE, MAP_SHARED,v4l2_fd, buf.m.offset);
        buffer_infos[n_buffers].length = buf.length;

        if (MAP_FAILED == buffer_infos[n_buffers].start) {
            printf("mmap error\n");
            return -1;
        }
    }
    printf("memory map success\n");

    /* 入队 */
    for (buf.index = 0; buf.index < FRAMEBUFFER_COUNT; buf.index++) {
        if (0 > ioctl(v4l2_fd, VIDIOC_QBUF, &buf)) {
            printf("入队失败\n");
            return -1;
        }
    }

    return 0;
}

int MainWindow::v4l2_stream_on(){
    /* 调用函数，初始化buffer */
    if(v4l2_init_buffer()<0){
        printf("------------------------------------\n");
        return -1;
    }

    /* 打开摄像头、摄像头开始采集数据 */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 > ioctl(v4l2_fd, VIDIOC_STREAMON, &type)) {
        printf("open stream failed\n");
        return -1;
    }
    printf("open stream success\n");

    return 0;
}

int MainWindow::v4l2_get_one_frame(FrameBuffer *framebuf){
    //初始化select()来进行I/O端口复用
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(v4l2_fd,&fds);

    //设置等待时间为2s
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    select(v4l2_fd+1,&fds,NULL,NULL,&tv);

    //将帧缓冲添加到队列
    struct v4l2_buffer one_buf;
    memset(&one_buf,0,sizeof(one_buf));
    one_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    one_buf.memory = V4L2_MEMORY_MMAP;

    // 从视频缓冲区的输出队列中取得一个已经保存有一帧视频数据的视频缓冲区
    if(0 > ioctl(v4l2_fd,VIDIOC_DQBUF,&one_buf)){
        printf("VIDIOC_DQBUF failed!\n");
        return -1;
    }

    //将buffer_infos中已使用的字节数copy到framebuf中
    memcpy(framebuf->buf,(char *)buffer_infos[one_buf.index].start,one_buf.bytesused);//bytesused 表示buf中已经使用的字节数
    framebuf->length = one_buf.bytesused;

    if(ioctl(v4l2_fd,VIDIOC_QBUF,&one_buf)==-1){
        printf("VIDIOC_QBUF failed\n");
        return -1;
    }

    return 0;
}

int MainWindow::v4l2_device_release(){
    /* 释放映射缓冲区 */
    int i;
    //关闭定时器
    timer->stop();
    for(i = 0; i < FRAMEBUFFER_COUNT; i++){
        munmap(buffer_infos[i].start,buffer_infos[i].length);
    }
    printf("------------------------------------\n");
    printf("munmap success\n");
    printf("camera release finished\n");

    return 0;
}
