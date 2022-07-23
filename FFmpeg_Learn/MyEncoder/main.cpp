#include "mainwindow.h"
#include <QApplication>

using namespace std;

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#define VIDEO_FPS 30

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile)
{
    int ret;
    /* send the frame to the encoder 发送帧到编码器 */
    /* pts：显示时间戳 打印出来*/
    if (frame)
        printf("Send frame %3" PRId64"\n", frame->pts);// PRId64 跨平台的书写方式，主要是为了同时支持32位和64位操作系统

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }
        printf("Write packet %3" PRId64" (size=%5d)\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

int main(int argc, char *argv[])
{
//    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();

    const char *filename;
    const AVCodec *codec;
    enum AVCodecID codec_id;
    AVCodecContext *c= NULL;
    int i, ret, x, y;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;
    /* MPEG文件结尾要添加的序列 */
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    /* 输入提示 */
    if (argc <= 1) {
        fprintf(stderr, "Usage: %s <output file>\n", argv[0]);
        exit(0);
    }
    filename = argv[1];
    /* 设置codecID */
    codec_id = AV_CODEC_ID_MPEG4;

    /* 根据codecID查找编码器 */
    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        printf("Codec '%d' not found\n", codec_id);
        exit(1);
    }
    /* 分配编码器上下文 AVCodecContext */
    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
    /* 分配 AVPacket，将每一帧 AVFrame 编码为 AVPacket */
    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    /* 给编码器上下文 AVCodecContext 配置一些参数 */
    /* 码率设置 */
    c->bit_rate = 400000;
    /* 视频宽高设置，参数必须为偶数 */
    c->width = 640;
    c->height = 480;
    /* 帧率设置 */
    c->time_base = (AVRational){1, VIDEO_FPS};
    c->framerate = (AVRational){VIDEO_FPS, 1};
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    /* GOP（Group of picture）图像组，指的是两个I帧之间的距离 */
    c->gop_size = 10;
    /* 最大B帧数 */
    c->max_b_frames = 1;
    /* 帧格式 */
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

    /* 打开编码器 */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        printf("Could not open codec\n");
        exit(1);
    }
    /* 打开输出文件，准备写入内容 */
    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    /* 分配 AVFrame 内存 */
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    /* 设置帧格式和宽高 */
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    /* 分配帧数据使用的空间 */
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }
    /* encode 1 second of video */
    for (i = 0; i < VIDEO_FPS; i++) {
        fflush(stdout);
        /* 确保帧 AVFrame 数据可写 */
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);

        /* 准备一帧虚拟图像 */
        /* Y */
        for (y = 0; y < c->height; y++) {
            for (x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }
        /* Cb and Cr */
        for (y = 0; y < c->height/2; y++) {
            for (x = 0; x < c->width/2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }
        /* 给每帧的pts赋值，告诉播放器在什么时候显示这一帧数据*/
        frame->pts = i;
        /* encode the image */
        encode(c, frame, pkt, f);
    }
    /* flush the encoder */
    encode(c, NULL, pkt, f);
    /* add sequence end code to have a real MPEG file */
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);
    /* 释放 AVCodecContext、AVFrame、AVPacket 占用的内存*/
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
//    return a.exec();
}
