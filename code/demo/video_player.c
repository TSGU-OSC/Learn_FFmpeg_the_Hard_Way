/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about palying(decoding and rendering) video through ffmpeg and SDL API 
 * 
 * FFmpeg version 6.0.1
 * SDL2 version 2.30.3
 *
 * Tested on MacOS 14.1.2, compiled with clang 14.0.3
 */

#include <SDL.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>

#define ONESECOND 1000

typedef struct VideoState{
    AVCodecContext *avctx;
    AVPacket       *pkt;
    AVFrame        *frame;
    AVStream       *stream;

    SDL_Texture    *texture;
}VideoState;


static int w_width = 1620;
static int w_height = 1080;

static SDL_Window *win = NULL;
static SDL_Renderer *renderer = NULL;

static void render(VideoState *is)
{

    SDL_UpdateYUVTexture(is->texture, NULL,
                         is->frame->data[0], is->frame->linesize[0] ,
                         is->frame->data[1], is->frame->linesize[1],
                         is->frame->data[2], is->frame->linesize[2]);
    
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, is->texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    int frameRate = is->stream->r_frame_rate.num/is->stream->r_frame_rate.den;
    if(frameRate <= 0){
        av_log(NULL, AV_LOG_ERROR,  "Failed to get framerate!\n");
        SDL_Delay(33);
        return;
    }
    SDL_Delay((Uint32)(ONESECOND/frameRate));
}


static int decode(VideoState *is)
{
    int ret = -1;

    char buffer[1024];
    //send packet to decoder
    ret = avcodec_send_packet(is->avctx, is->pkt);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
        goto end;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(is->avctx, is->frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            ret = 0;
            goto end;
        }else if(ret < 0){
            ret = -1;
            goto end;
        }
        render(is);
    }
    

end:
    return ret;
}

static int open_media()
{

}

int main(int argc, char *argv[])
{

    int ret = -1;
    int idx = -1;
    AVFormatContext *fmtCtx = NULL;
    AVStream *inStream = NULL;
    const AVCodec *decodec = NULL;
    AVCodecContext *ctx = NULL;

    SDL_Texture *texture = NULL;
    SDL_Event event;

    Uint32 pixformat = 0;

    int video_height = 0;
    int video_width = 0;

    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;

    VideoState *is = NULL; 
    
    //deal with arguments
    char *src;

    av_log_set_level(AV_LOG_DEBUG);

    if(argc < 2){
        av_log(NULL, AV_LOG_ERROR, "the arguments must be more than 2!\n");
        exit(-1);
    }

    src = argv[1];
    
    is = av_mallocz(sizeof(VideoState));
    if(!is){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        goto end;
    }

    //init SDL
    if (SDL_Init(SDL_INIT_VIDEO)){
        fprintf(stderr, "Couldn't initialize SDL - %s\n", SDL_GetError);
        return -1;
    }
    //create window from SDL
    win = SDL_CreateWindow("simple player",
                           SDL_WINDOWPOS_UNDEFINED,
                           SDL_WINDOWPOS_UNDEFINED,
                           w_width, w_height,
                           SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!win){
        fprintf(stderr, "Failed to create window, %s\n", SDL_GetError);
        goto end;
    }   
    renderer = SDL_CreateRenderer(win, -1, 0);

    //open multimedia file and get stream info
    if( (ret = avformat_open_input(&fmtCtx, src, NULL, NULL)) < 0 ){
        av_log(NULL, AV_LOG_ERROR, " %s \n", av_err2str(ret));
        goto end;
    }
    if((ret = avformat_find_stream_info(fmtCtx, NULL)) < 0){
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        goto end;
    }
    //find the best stream
    if((idx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0){
        av_log(fmtCtx, AV_LOG_ERROR, "There is no audio stream!\n");
        goto end;
    }
    //get decodec by codec_id from stream info
    inStream = fmtCtx->streams[idx];
    decodec = avcodec_find_decoder(inStream->codecpar->codec_id);
    if(!decodec){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: libx264 \n");
        goto end;
    }
    is->stream = inStream;

    //init decoder context
    ctx = avcodec_alloc_context3(decodec);
    if(!ctx){
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        goto end;
    }
    //copy parameters 
    avcodec_parameters_to_context(ctx, inStream->codecpar);

    //bind decoder and decoder context
    ret = avcodec_open2(ctx, decodec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        goto end;
    }
    //create texture for render 
    video_width = ctx->width;
    video_height = ctx->height;
    pixformat = SDL_PIXELFORMAT_IYUV;
    texture = SDL_CreateTexture(renderer, pixformat, SDL_TEXTUREACCESS_STREAMING, video_width, video_height);

    pkt = av_packet_alloc();
    frame = av_frame_alloc();
    
    is->texture = texture;
    is->avctx = ctx;
    is->pkt = pkt;
    is->frame = frame;
    
    //decode
    while(av_read_frame(fmtCtx, pkt) >= 0){
        if(pkt->stream_index == idx ){
            //render
            decode(is);
        }
        //deal with SDL event
        
        SDL_PollEvent(&event);
        switch (event.type)
        {
        case SDL_QUIT:
            goto quit;
            break;
        
        default:
            break;
        }

        av_packet_unref(pkt);
    
    }
    is->pkt = NULL;
    decode(is);

quit:
    ret = 0;
end:
    if(frame){
        av_frame_free(&frame);
    }
    if (pkt){
        av_packet_free(&pkt);
    }
    if(ctx){
        avcodec_free_context(&ctx);
    }
    if(fmtCtx){
        avformat_close_input(&fmtCtx);
    }
    if(win){
        SDL_DestroyWindow(win);
    }
    if(renderer){
        SDL_DestroyRenderer(renderer);
    }
    if(texture){
        SDL_DestroyTexture(texture);
    }
    SDL_Quit();    
    return ret;
}