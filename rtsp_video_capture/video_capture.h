#pragma once
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef __cplusplus
extern "C"
{
#endif
/*Include ffmpeg header file*/
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>

#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

#ifdef __cplusplus
}
#endif

class VideoCapture
{
public:
    explicit VideoCapture(const std::string &filename);
    ~VideoCapture();
    bool isOpened() { return _isOpened; };
    VideoCapture &operator>>(cv::Mat &image);

private:
    VideoCapture(){};
    bool open(const std::string &filename);
    bool read(cv::Mat &image);
    void release();

private:
    mutable bool _isOpened{false};

    AVCodec *_codec{NULL};
    AVCodecContext *_ctx{NULL};
    AVCodecParameters *_origin_par{NULL};
    AVFrame *_fr{NULL};
    AVFrame *_frBGR{NULL};
    AVPacket *_pkt{NULL};
    AVFormatContext *_fmt_ctx{NULL};
    struct SwsContext *_img_convert_ctx{NULL};
    uint8_t *_out_buffer{NULL};
    int _size{-1};
    int _video_stream{-1};
};
