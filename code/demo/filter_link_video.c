/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about multi filtering video through ffmpeg API
 * 
 * FFmpeg version 5.1.4
 * Tested on MacOS 14.7.1, compiled with clang 16.0.0
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>

#define CHECK_ERROR(err) \
    if ((err) < 0) { \
        char errbuf[128]; \
        av_strerror((err), errbuf, sizeof(errbuf)); \
        fprintf(stderr, "Error: %s\n", errbuf); \
        exit(1); \
    }

const char *filter_descr = 
    "[in0]pad=iw*2:ih[int];[int][in1]overlay=w[out]";

static AVFormatContext *fmt_ctx1;
static AVFormatContext *fmt_ctx2;
static AVCodecContext *dec_ctx1;
static AVCodecContext *dec_ctx2;

AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx1;
AVFilterContext *buffersrc_ctx2;
AVFilterGraph *filter_graph;

static int video_stream_index1 = -1;
static int video_stream_index2 = -1;

int width      = 640;
int height     = 480;
enum AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
AVRational sample_aspect_ratio = {1, 1};

static void save_pgm(unsigned char* buffer, int linesize, int width, int height, char *name)
{
    FILE *f;

    f = fopen(name, "wb");
    fprintf(f, "P5\n%d %d\n%d\n", width, height, 255);
    for (int i = 0; i < height; i++){
        fwrite(buffer + i * linesize, 1, width, f);
    }

    fclose(f);
    
}

static int open_input_file(const char *filename, AVFormatContext **fmt_ctx, AVCodecContext **dec_ctx, int *video_stream_index)
{
    const AVCodec *dec;
    int ret;

    if ((ret = avformat_open_input(fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(*fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the video stream */
    ret = av_find_best_stream(*fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    *video_stream_index = ret;

    /* create decoding context */
    *dec_ctx = avcodec_alloc_context3(dec);
    if (!*dec_ctx)
        return AVERROR(ENOMEM);
    avcodec_parameters_to_context(*dec_ctx, (*fmt_ctx)->streams[*video_stream_index]->codecpar);

    /* init the video decoder */
    if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}

static int init_filters(const char *filters_descr, AVRational time_base, int width, int height, enum AVPixelFormat pix_fmt, AVRational sample_aspect_ratio)
{
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };

    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            width, height, pix_fmt,
            time_base.num, time_base.den,
            sample_aspect_ratio.num, sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx1, buffersrc, "in0",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source 1\n");
        goto end;
    }
    ret = avfilter_graph_create_filter(&buffersrc_ctx2, buffersrc, "in1",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source 2\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
        goto end;
    }

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    AVFilterInOut *inputs  = NULL;
    AVFilterInOut *outputs = NULL;


    inputs = avfilter_inout_alloc();
    if (!inputs) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next = NULL;

    outputs = avfilter_inout_alloc();
    if (!outputs) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    outputs->name       = av_strdup("in0");
    outputs->filter_ctx = buffersrc_ctx1;
    outputs->pad_idx    = 0;
    outputs->next       = avfilter_inout_alloc();
    if (!outputs->next) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    outputs->next->name = av_strdup("in1");
    outputs->next->filter_ctx = buffersrc_ctx2;
    outputs->next->pad_idx    = 0;
    outputs->next->next = NULL;

    // if ((ret = avfilter_graph_parse2(filter_graph, filter_descr, inputs, outputs)) < 0)
    //     goto end;
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr, &inputs, &outputs, NULL)) < 0)
        goto end;
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return ret;
}

int main(int argc, char **argv)
{
    int ret;
    AVPacket packet1, packet2;
    AVFrame *frame1, *frame2;
    AVFrame *filt_frame;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s file1 file2\n", argv[0]);
        exit(1);
    }

    frame1 = av_frame_alloc();
    frame2 = av_frame_alloc();
    filt_frame = av_frame_alloc();
    av_init_packet(&packet1);
    av_init_packet(&packet2);
    if (!frame1 || !frame2 || !filt_frame || !&packet1 || !&packet2) {
        fprintf(stderr, "Could not allocate frame or packet\n");
        exit(1);
    }

    if ((ret = open_input_file(argv[1], &fmt_ctx1, &dec_ctx1, &video_stream_index1)) < 0)
        goto end;
    if ((ret = open_input_file(argv[2], &fmt_ctx2, &dec_ctx2, &video_stream_index2)) < 0)
        goto end;
    
    AVRational time_base = fmt_ctx1->streams[video_stream_index1]->time_base;
    width = fmt_ctx1->streams[video_stream_index1]->codecpar->width;
    height = fmt_ctx1->streams[video_stream_index1]->codecpar->height;
    if ((ret = init_filters(filter_descr, time_base, width, height, pix_fmt, sample_aspect_ratio)) < 0)
        goto end;

    char *fileName = argv[3];
    AVFrame *video_dst = av_frame_alloc();
    int frameNumber = 0;

    int finished1 = 0, finished2 = 0;
    /* read all packets */
    while (!finished1 || !finished2) {
        // Process video1
        if (!finished1 && av_read_frame(fmt_ctx1, &packet1) >= 0) {
            if (packet1.stream_index == video_stream_index1) {
                // Send the packet to the decoder.
                if (avcodec_send_packet(dec_ctx1, &packet1) == 0) {
                    // Receive all available frames.
                    while (avcodec_receive_frame(dec_ctx1, frame1) == 0) {
                        // Feed the frame into the filter graph for input 1.
                        if (av_buffersrc_add_frame_flags(buffersrc_ctx1, frame1, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                            fprintf(stderr, "Error while feeding frame to filter graph (video1)\n");
                        }
                        av_frame_unref(frame1);
                    }
                }
            }
            av_packet_unref(&packet1);
        } else {
            finished1 = 1;
            // Optionally send a flush signal to the decoder for video1.
        }

        // Process video2
        if (!finished2 && av_read_frame(fmt_ctx2, &packet2) >= 0) {
            if (packet2.stream_index == video_stream_index2) {
                if (avcodec_send_packet(dec_ctx2, &packet2) == 0) {
                    while (avcodec_receive_frame(dec_ctx2, frame2) == 0) {
                        // Feed the frame into the filter graph for input 2.
                        if (av_buffersrc_add_frame_flags(buffersrc_ctx2, frame2, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                            fprintf(stderr, "Error while feeding frame to filter graph (video2)\n");
                        }
                        av_frame_unref(frame2);
                    }
                }
            }
            av_packet_unref(&packet2);
        } else {
            finished2 = 1;
            // Optionally flush decoder for video2.
        }

        // Try to pull a filtered frame from the sink.
        int ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // av_frame_free(&filt_frame);
            continue;
        } else if (ret < 0) {
            fprintf(stderr, "Error retrieving frame from filter graph\n");
            av_frame_free(&filt_frame);
            break;
        }
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "%s-%d.pgm", fileName, frameNumber++);
        save_pgm(filt_frame->data[0], filt_frame->linesize[0], filt_frame->width, filt_frame->height, buffer);

    }
end:
    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&dec_ctx1);
    avcodec_free_context(&dec_ctx2);
    avformat_close_input(&fmt_ctx1);
    avformat_close_input(&fmt_ctx2);
    av_frame_free(&frame1);
    av_frame_free(&frame2);
    av_frame_free(&filt_frame);
    av_packet_free(&packet1);
    av_packet_free(&packet2);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }

    exit(0);
}