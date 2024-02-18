/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about decoding video through ffmpeg API
 * 
 * FFmpeg version 5.0.3 
 * Tested on MacOS 14.1.2, compiled with clang 14.0.3
 */

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>

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

static int decode(AVCodecContext *ctx, AVFrame *frame,AVPacket *pkt, const char *fileName)
{
    int ret = -1;

    char buffer[1024];
    //send packet to decoder
    ret = avcodec_send_packet(ctx, pkt);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
        goto end;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(ctx, frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){

            return -1;
        }
        snprintf(buffer, sizeof(buffer), "%s-%d", fileName, ctx->frame_number);

        save_pgm(frame->data[0],
                 frame->linesize[0],
                 frame->width,
                 frame->height,
                 buffer);
        
        if (pkt)
            av_packet_unref(pkt);
    }
    

end:
    return 0;
}


int main(int argc, char *argv[])
{
    int ret = -1;
    int idx = -1;
    //deal with arguments
    char *src;
    char *dst;

    AVFormatContext *pFmtCtx = NULL;

    AVStream *inStream = NULL;

    AVPacket *pkt = NULL;

    const AVCodec *codec = NULL;
    AVCodecContext *ctx = NULL;

    AVFrame *frame = NULL;

    av_log_set_level(AV_LOG_DEBUG);
    if(argc < 3){
        av_log(NULL, AV_LOG_ERROR, "the arguments must be more than 3!\n");
    }

    src = argv[1];
    dst = argv[2];
    
    //open the multimedia file
    if( (ret = avformat_open_input(&pFmtCtx, src, NULL, NULL)) < 0 ){
        av_log(NULL, AV_LOG_ERROR, " %s \n", av_err2str(ret));
        exit(-1);
    }

    //find the video stream from container
    if((idx = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0){
        av_log(pFmtCtx, AV_LOG_ERROR, "There is no audio stream!\n");
        goto end;
    }

    inStream = pFmtCtx->streams[idx];

    //find the decoder
    //codec = avcodec_find_encoder_by_name(codecID);
    //find the decoder by ID

    codec = avcodec_find_decoder(inStream->codecpar->codec_id);
    if(!codec){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: libx264 \n");
        goto end;
    }

    //init decoder context
    ctx = avcodec_alloc_context3(codec);
    if(!ctx){
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        goto end;
    }

    avcodec_parameters_to_context(ctx, inStream->codecpar);

    //bind decoder and decoder context
    ret = avcodec_open2(ctx, codec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        goto end;
    }

    //create AVFrame
    frame = av_frame_alloc();
    if(!frame){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        goto end;
    }

   

    //create AVPacket
    pkt = av_packet_alloc();
    if(!pkt){
        av_log(NULL, AV_LOG_ERROR, "NO Memory!\n");
        goto end;
    }

    

    //read video data from multimedia files to write into destination file
    while(av_read_frame(pFmtCtx, pkt) >= 0){
        if(pkt->stream_index == idx ){
            decode(ctx, frame, pkt, dst);
        }
    }
    //write the buffered frame
    decode(ctx, frame, NULL, dst);
    

    //free memory
end:
    if(pFmtCtx){
        avformat_close_input(&pFmtCtx);
        pFmtCtx = NULL;
    }
    if(ctx){
        avcodec_free_context(&ctx);
        ctx = NULL;
    }
    if (frame){
        av_frame_free(&frame);
        frame = NULL;
    }
    if (pkt){
        av_packet_free(&pkt);
        pkt = NULL;
    }
    
    

    return 0;
}