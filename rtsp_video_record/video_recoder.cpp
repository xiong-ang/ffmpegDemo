#include "video_recoder.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
}
#endif

bool VideoRecoder::Start()
{
    _recording = true;
    _handler_future = std::async(std::launch::async, &VideoRecoder::StartHandle, this);
    return true;
}

bool VideoRecoder::Stop()
{
    _recording = false;
    return _handler_future.get();
}

bool VideoRecoder::StartHandle()
{
    avcodec_register_all();
    av_register_all();
    avformat_network_init();

    AVFormatContext *i_fmt_ctx;
    AVStream *i_video_stream;

    AVFormatContext *o_fmt_ctx;
    AVStream *o_video_stream;

    /* should set to NULL so that avformat_open_input() allocate a new one */
    i_fmt_ctx = NULL;
    int ret = avformat_open_input(&i_fmt_ctx, _rtsp_url.c_str(), NULL, NULL);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't open rtsp capture\n");
        avformat_close_input(&i_fmt_ctx);
        return false;
    }

    ret = avformat_find_stream_info(i_fmt_ctx, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to retrieve input stream information\n");
        avformat_close_input(&i_fmt_ctx);
        return false;
    }

    int i_video_stream_index = av_find_best_stream(i_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (i_video_stream_index < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't find video stream in input file\n");
        avformat_close_input(&i_fmt_ctx);
        return false;
    }
    i_video_stream = i_fmt_ctx->streams[i_video_stream_index];
    /*
    for (unsigned i = 0; i < i_fmt_ctx->nb_streams; i++)
    {
        if (i_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            i_video_stream = i_fmt_ctx->streams[i];
            break;
        }
    }
    if (i_video_stream == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "Can't find video stream in input file\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    */

    ret = avformat_alloc_output_context2(&o_fmt_ctx, NULL, NULL, _file_name.c_str());
    if (ret < 0 || !o_fmt_ctx)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        avformat_close_input(&i_fmt_ctx);
        av_free(o_fmt_ctx);
        return false;
    }

    o_fmt_ctx->probesize = 10000000;
	o_fmt_ctx->flags |= AVFMT_FLAG_NOBUFFER;
	av_opt_set(o_fmt_ctx->priv_data,"preset","ultrafast",0);
	o_fmt_ctx->max_analyze_duration = 5 * AV_TIME_BASE;

    /*
    * since all input files are supposed to be identical (framerate, dimension, color format, ...)
    * we can safely set output codec values from first input file
    */
    o_video_stream = avformat_new_stream(o_fmt_ctx, NULL);
    if (!o_video_stream)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream.\n");
        avformat_close_input(&i_fmt_ctx);
        av_free(o_fmt_ctx);
        return false;
    }
    {
        AVCodecContext *c;
        c = o_video_stream->codec;
        c->bit_rate = i_video_stream->codec->bit_rate; //400000;
        c->codec_id = i_video_stream->codec->codec_id;
        c->codec_type = i_video_stream->codec->codec_type;
        c->time_base.num = i_video_stream->time_base.num;
        c->time_base.den = i_video_stream->time_base.den;
        c->width = i_video_stream->codec->width;
        c->height = i_video_stream->codec->height;
        c->pix_fmt = i_video_stream->codec->pix_fmt;
        c->flags = i_video_stream->codec->flags;
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        c->me_range = i_video_stream->codec->me_range;
        c->max_qdiff = i_video_stream->codec->max_qdiff;

        c->qmin = i_video_stream->codec->qmin;
        c->qmax = i_video_stream->codec->qmax;

        c->qcompress = i_video_stream->codec->qcompress;
    }

    ret = avio_open(&o_fmt_ctx->pb, _file_name.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not open output file\n");
        avformat_close_input(&i_fmt_ctx);
        av_free(o_fmt_ctx);
        avcodec_close(o_fmt_ctx->streams[0]->codec);
        av_freep(&o_fmt_ctx->streams[0]->codec);
        av_freep(&o_fmt_ctx->streams[0]);
        return false;
    }

    ret = avformat_write_header(o_fmt_ctx, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not open output file\n");
        avformat_close_input(&i_fmt_ctx);
        av_free(o_fmt_ctx);
        avcodec_close(o_fmt_ctx->streams[0]->codec);
        av_freep(&o_fmt_ctx->streams[0]->codec);
        av_freep(&o_fmt_ctx->streams[0]);
        return false;
    }

    int64_t last_pts = 0;
    int64_t last_dts = 0;
    bool isFirstFrame = true;
    int64_t pts, dts;
    while (_recording)
    {
        AVPacket i_pkt;
        av_init_packet(&i_pkt);
        i_pkt.size = 0;
        i_pkt.data = NULL;
        if (av_read_frame(i_fmt_ctx, &i_pkt) < 0)
        {
            av_packet_unref(&i_pkt);
            break;
        }

        /*
        * pts and dts should increase monotonically
        * pts should be >= dts
        */
        if (i_pkt.dts < 0 || i_pkt.pts < 0 || i_pkt.dts > i_pkt.pts || isFirstFrame)
        {
            isFirstFrame = false;
            i_pkt.dts = i_pkt.pts = i_pkt.duration = 0;
        }
        i_pkt.flags |= AV_PKT_FLAG_KEY;
        pts = i_pkt.pts;
        i_pkt.pts += last_pts;
        dts = i_pkt.dts;
        i_pkt.dts += last_dts;
        i_pkt.stream_index = 0;

        av_interleaved_write_frame(o_fmt_ctx, &i_pkt);
        av_packet_unref(&i_pkt);
    }
    last_dts += dts;
    last_pts += pts;

    avformat_close_input(&i_fmt_ctx);

    av_write_trailer(o_fmt_ctx);

    avcodec_close(o_fmt_ctx->streams[0]->codec);
    av_freep(&o_fmt_ctx->streams[0]->codec);
    av_freep(&o_fmt_ctx->streams[0]);

    avio_close(o_fmt_ctx->pb);
    av_free(o_fmt_ctx);
    return true;
}