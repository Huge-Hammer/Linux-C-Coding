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
#include <linux/videodev2.h>
#include <linux/fb.h>

int main(int argc, char *argv[]){
    if (2 != argc) {
        fprintf(stderr, "Usage: %s <video_dev>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // 打开设备
    int fd;
    fd = open(argv[1], O_RDWR);
    if(fd < 0){
        perror("video设备打开失败\n");
        return -1;
    }
    else{
        printf("video设备打开成功\n");
    }

    //查看设备是否为视频采集设备
    ioctl(fd, VIDIOC_QUERYCAP, &vcap);
    if (!(V4L2_CAP_VIDEO_CAPTURE & vcap.capabilities)) {
        perror("Error: No capture video device!\n");
        return -1;
    }

    // 枚举帧格式
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("摄像头支持所有格式如下:\n");
    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc) == 0){
        printf("v4l2_format%d:%s\n",fmtdesc.index,fmtdesc.description);
        fmtdesc.index++;
    }

    // 枚举分辨率
    struct v4l2_frmsizeenum frmsize;
    frmsize.index = 0;
    frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("MJPEG格式支持所有分辨率如下:\n");
    // frmsize.pixel_format = V4L2_PIX_FMT_YUYV;
    frmsize.pixel_format = V4L2_PIX_FMT_MJPEG;
    while(ioctl(fd,VIDIOC_ENUM_FRAMESIZES,&frmsize) == 0){
        printf("frame_size<%d*%d>\n",frmsize.discrete.width,frmsize.discrete.height);
        frmsize.index++;
    }
    
    // 枚举某分辨率下的帧速率
    struct v4l2_frmivalenum frmival;
    frmival.index = 0;
    frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    frmival.pixel_format = V4L2_PIX_FMT_MJPEG;
    frmival.width = 640;
    frmival.height = 480;
    while(ioctl(fd,VIDIOC_ENUM_FRAMEINTERVALS,&frmival) == 0){
        printf("frame_interval under frame_size <%d*%d> support %dfps\n",frmival.width,frmival.height,frmival.discrete.denominator / frmival.discrete.numerator);
        frmival.index++;
    }
 
    // 设置采集格式
    struct v4l2_format vfmt;
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vfmt.fmt.pix.width = 640;
    vfmt.fmt.pix.height = 480;
    vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    if(ioctl(fd,VIDIOC_S_FMT,&vfmt) < 0){
        perror("设置格式失败\n");
        return -1;
    }
    // 检查设置参数是否生效
    if(ioctl(fd,VIDIOC_G_FMT,&vfmt) < 0){
        perror("获取设置格式失败\n");
        return -1;
    }
    else if(vfmt.fmt.pix.width == 640 && vfmt.fmt.pix.height == 480 && vfmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG){
        printf("设置格式生效,实际分辨率大小<%d * %d>,图像格式:Motion-JPEG\n",vfmt.fmt.pix.width,vfmt.fmt.pix.height);
    }
    else{
        printf("设置格式未生效\n");
    }

    // 获取帧信息
    struct v4l2_streamparm streamparm;
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_G_PARM, &streamparm);

    if(V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability){
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 30; //30fps     
        if(ioctl(fd,VIDIOC_S_PARM, &streamparm) < 0){
            printf("设置帧率失败\n");
            return -1;
        }
    }
    else printf("该摄像头不支持设置帧率\n");

    // 申请缓冲区空间
    struct v4l2_requestbuffers reqbuf;
    reqbuf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.count = 3;   //3个帧缓冲
    reqbuf.memory = V4L2_MEMORY_MMAP;
    if(ioctl(fd,VIDIOC_REQBUFS,&reqbuf) < 0){
        perror("申请缓冲区失败\n");
        return -1;
    }

    // 将帧缓冲映射到进程地址空间
    void *frm_base[3];  //映射后的用户空间的首地址
    unsigned int frm_size[3];

    struct v4l2_buffer buf;
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    // 将每一帧对应的缓冲区的起始地址保存在frm_base数组中，读取采集数据时，只需直接读取映射区即可
    for(buf.index=0;buf.index<3;buf.index++){
        ioctl(fd, VIDIOC_QUERYBUF, &buf);
        frm_base[buf.index] = mmap(NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED,fd,buf.m.offset);
        frm_size[buf.index] = buf.length;

        if(frm_base[buf.index] == MAP_FAILED){
            perror("mmap failed\n");
            return -1;
        }

        // 入队操作
        if(ioctl(fd,VIDIOC_QBUF,&buf) < 0){
            perror("入队失败\n");
            return -1;
        }
    }

    // 开始采集
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0){
        perror("开始采集失败\n");
        return -1;
    }
    
    // 读取帧
    struct v4l2_buffer  readbuffer;
    readbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    readbuffer.memory = V4L2_MEMORY_MMAP;
    if(ioctl(fd, VIDIOC_DQBUF, &readbuffer) < 0){
        perror("读取帧失败\n");
    }

    // 保存这一帧，格式为jpg
    FILE *file = fopen("v4l2_cap.jpg","w+");
    fwrite(frm_base[readbuffer.index],readbuffer.length,1,file);
    fclose(file);

    // 再次入队
    if(ioctl(fd,VIDIOC_QBUF,&readbuffer) < 0){
        perror("入队失败\n");
    }

    // 停止采集
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0){
        perror("停止采集失败\n");
        return -1;
    }

    // 释放映射
    for(int i=0;i<3;i++){
        munmap(frm_base[i],frm_size[i]);
    }

    close(fd);
    return 0;
}