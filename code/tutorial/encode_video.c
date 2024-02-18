/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about encoding video through ffmpeg API
 * 
 * FFmpeg version 5.0.3 
 * Tested on MacOS 14.1.2, compiled with clang 14.0.3
 */
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/log.h>

static int encode(AVCodecContext *ctx, AVFrame *frame,AVPacket *pkt, FILE *file)
{
    int ret = -1;
    //send frame to encoder
    ret = avcodec_send_frame(ctx, frame);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to encoder!\n");
        goto end;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(ctx, pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){

            return -1;
        }

        fwrite(pkt->data, 1, pkt->size, file);
        av_packet_unref(pkt);
    }
    

end:
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    FILE *f = NULL;
    
    int codecID = 0;
    char *dst = NULL;

    const AVCodec *codec = NULL;
    AVCodecContext *ctx = NULL;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;

    av_log_set_level(AV_LOG_DEBUG);

    //input arguments
    if(argc < 3){
        av_log(NULL, AV_LOG_ERROR, "The arguments must be more than 3!\n");
        goto end;
    }

    dst = argv[1];
    codecID = atoi(argv[2]);
    
    //find the encodec
    //codec = avcodec_find_encoder_by_name(codecID);
    //find the encodec by ID
    codec = avcodec_find_encoder(codecID);
    if(!codec){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: %d\n", codecID);
        goto end;
    }

    //init codec context
    ctx = avcodec_alloc_context3(codec);
    if(!ctx){
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        goto end;
    }
    //set parameters of codec
    ctx->width = 640;
    ctx->height = 480;
    ctx->bit_rate = 500000;
    
    ctx->time_base = (AVRational){1, 25};
    ctx->framerate = (AVRational){25, 1};

    ctx->gop_size = 10;
    ctx->max_b_frames = 1;
    ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    //set the 

    if(codec->id == AV_CODEC_ID_H264){
        av_opt_set(ctx->priv_data, "preset", "slow", 0);
    }

    //bind codec and codec context
    ret = avcodec_open2(ctx, codec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        goto end;
    }

    //create output file
    f = fopen(dst, "wb");
    if(!f){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open file: %s\n", dst);
        goto end;
    }

    //create AVFrame
    frame = av_frame_alloc();
    if(!frame){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        goto end;
    }

    frame->width = ctx->width;
    frame->height = ctx->height;
    frame->format = ctx->pix_fmt;

    ret = av_frame_get_buffer(frame, 0);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't allocate the video frame\n");
        goto end;
    }

    //create AVPacket
    pkt = av_packet_alloc();
    if(!pkt){
        av_log(NULL, AV_LOG_ERROR, "NO Memory!\n");
        goto end;
    }

    //create video data
    for (int i = 0; i < 25; i++)
    {
        ret = av_frame_is_writable(frame);
        if(ret < 0){
            break;
        }
        //Y
        for (int y = 0; y < ctx->height; y++)
        {
            for(int x = 0; x < ctx->width; x++){
                frame->data[0][y * frame->linesize[0]+x] = x + y + i * 3;
            }
        }
        //UV
        for (int y = 0; y < ctx->height/2; y++)
        {
            for(int x = 0; x < ctx->width/2; x++){
                frame->data[1][y * frame->linesize[1]+x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2]+x] = 64 + y + i * 6;
            }
        }

        frame->pts = i;

        //encode
        ret = encode(ctx, frame, pkt, f);
        if (ret == -1){
            goto end;
        }
    }
    //encode the buffered frame
    encode(ctx, NULL, pkt, f);
    av_log(NULL, AV_LOG_INFO, "Encode Success!\n");

    
    

end:
    //free memory
    if(ctx){
        avcodec_free_context(&ctx);
    }

    if(frame){
        av_frame_free(&frame);
    }

    if(pkt){
        av_packet_free(&pkt);
    }

    if(f){
        fclose(f);
    }


    return 0;
}