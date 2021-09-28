#include "video_capture.h"
#include <iostream>

VideoCapture::VideoCapture(const std::string &filename)
{
    _isOpened = open(filename);
};

VideoCapture::~VideoCapture()
{
    release();
}

bool VideoCapture::open(const std::string &filename)
{
    AVDictionary *options = NULL;
    av_dict_set(&options, "buffer_size", "1024000", 0); //设置缓存大小,1080p可将值跳到最大
    av_dict_set(&options, "rtsp_transport", "tcp", 0);  //以tcp的方式打开,
    av_dict_set(&options, "stimeout", "5000000", 0);    //设置超时断开链接时间，单位us
    av_dict_set(&options, "max_delay", "500000", 0);    //设置最大时延
    int result = avformat_open_input(&_fmt_ctx, filename.c_str(), NULL, &options);
    if (result < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't open file\n");
        return false;
    }

    result = avformat_find_stream_info(_fmt_ctx, NULL);
    if (result < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't get stream info\n");
        return false;
    }

    _video_stream = av_find_best_stream(_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (_video_stream < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't find video stream in input file\n");
        return false;
    }

    _origin_par = _fmt_ctx->streams[_video_stream]->codecpar;
    _codec = avcodec_find_decoder(_origin_par->codec_id);
    if (!_codec)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't find decoder\n");
        return false;
    }

    _ctx = avcodec_alloc_context3(_codec);
    if (!_ctx)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate decoder context\n");
        return false;
    }

    result = avcodec_parameters_to_context(_ctx, _origin_par);
    if (result)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't copy decoder context\n");
        return false;
    }

    result = avcodec_open2(_ctx, _codec, NULL);
    if (result < 0)
    {
        av_log(_ctx, AV_LOG_ERROR, "Can't open decoder\n");
        return false;
    }

    _fr = av_frame_alloc();
    if (!_fr)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate frame\n");
        return false;
    }

    _frBGR = av_frame_alloc();
    if (!_frBGR)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't allocate frame\n");
        return false;
    }

    _size = avpicture_get_size(AV_PIX_FMT_BGR24, _ctx->width, _ctx->height);
    _out_buffer = (uint8_t *)av_malloc(_size);
    avpicture_fill((AVPicture *)_frBGR, _out_buffer, AV_PIX_FMT_BGR24, _ctx->width, _ctx->height);
    _img_convert_ctx = sws_getContext(_ctx->width, _ctx->height, _ctx->pix_fmt, _ctx->width, _ctx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

    _pkt = av_packet_alloc();
    if (!_pkt)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot allocate packet\n");
        return false;
    }
    return true;
}

bool VideoCapture::read(cv::Mat &image)
{
    if (!_isOpened)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot open file\n");
        return false;
    }

    int i = 0;
    int result = 0;
    while (result >= 0)
    {
        result = av_read_frame(_fmt_ctx, _pkt);
        if (result >= 0 && _pkt->stream_index != _video_stream)
        {
            av_packet_unref(_pkt);
            continue;
        }

        if (result < 0)
            result = avcodec_send_packet(_ctx, NULL);
        else
        {
            if (_pkt->pts == AV_NOPTS_VALUE)
                _pkt->pts = _pkt->dts = i;
            result = avcodec_send_packet(_ctx, _pkt);
        }
        av_packet_unref(_pkt);

        if (result < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Error submitting a packet for decoding\n");
            return false;
        }

        while (result >= 0)
        {
            result = avcodec_receive_frame(_ctx, _fr);
            if (result == AVERROR_EOF)
            {
                av_log(NULL, AV_LOG_ERROR, "End of file\n");
                return false;
            }
            if (result == AVERROR(EAGAIN))
            {
                result = 0;
                break;
            }
            if (result < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "Error decoding frame\n");
                return false;
            }

            sws_scale(_img_convert_ctx, (const uint8_t *const *)_fr->data, _fr->linesize, 0, _ctx->height, _frBGR->data, _frBGR->linesize); //YUV to RGB
            cv::Mat(_ctx->height, _ctx->width, CV_8UC3, _out_buffer).copyTo(image);
            av_frame_unref(_fr);
            return true;
        }
        i++;
    }
    return true;
}

VideoCapture &VideoCapture::operator>>(cv::Mat &image)
{
    if (!read(image))
        image = cv::Mat();
    return *this;
}

void VideoCapture::release()
{
    av_packet_free(&_pkt);
    av_frame_free(&_fr);
    av_frame_free(&_frBGR);
    avformat_close_input(&_fmt_ctx);
    avcodec_free_context(&_ctx);
    av_freep(&_out_buffer);
}