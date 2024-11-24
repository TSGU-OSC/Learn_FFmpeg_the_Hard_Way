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
#include <libswresample/swresample.h>
#include <libavutil/fifo.h>

#include <time.h>
#include <pthread.h>


#define ONESECOND 1000
#define AUDIO_BUFFER_SIZE 1024

typedef struct MyPacketEle{
    AVPacket *pkt;
}MyPacketEle;

typedef struct PacketQueue{
    AVFifo *pkts;
    int nb_packets;
    int size;
    int64_t duration;
    SDL_mutex *mutex;
    SDL_cond *cond;
}PacketQueue;

typedef struct VideoState{
    AVFormatContext *fmtCtx;

    AVCodecContext *aCtx;
    AVCodecContext *vCtx;
    AVPacket       *aPkt;
    AVPacket       *vPkt;
    AVFrame        *aFrame;
    AVFrame        *vFrame;

    int            vIdx;
    int            aIdx;

    struct SwrContext *swr_ctx;

    uint8_t        *audio_buf;
    uint           audio_buf_size;
    int            audio_buf_index;

    SDL_Texture    *texture;

    PacketQueue    audioQueue;
}VideoState;

static int w_width = 640;
static int w_height = 480;

static SDL_Window *win = NULL;
static SDL_Renderer *renderer = NULL;

// pthread_mutex_t mutex;
// pthread_cond_t  cond;
SDL_mutex       *videoMutex;
SDL_cond        *videoCond;
int             frame_decoded;

// Define the function pointer type for the function to be measured
typedef int (*FunctionPtr)(VideoState *is);

void timer(FunctionPtr function, VideoState *is) {
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    function(is);
    end = clock();

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Function executed in %f seconds\n", cpu_time_used);
}

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

static int packet_queue_init(PacketQueue *q)
{
    memset(q, 0, sizeof(PacketQueue));
    q->pkts = av_fifo_alloc2(1, sizeof(MyPacketEle), AV_FIFO_FLAG_AUTO_GROW);
    if(!q->pkts){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        return AVERROR(ENOMEM);
    }
    q->mutex = SDL_CreateMutex();
    if(!q->mutex){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        return AVERROR(ENOMEM);
    }
    q->cond = SDL_CreateCond();
    if(!q->cond){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}

static int packet_queue_put_priv(PacketQueue *q, AVPacket *pkt)
{
    MyPacketEle mypkt;
    int ret = -1;

    mypkt.pkt = pkt;

    ret = av_fifo_write(q->pkts, &mypkt, 1);
    if(ret < 0){
        return ret;
    }
    //update the queue info
    q->nb_packets++;
    q->size += mypkt.pkt->size + sizeof(mypkt);
    q->duration += mypkt.pkt->duration;
    //
    SDL_CondSignal(q->cond);

    return ret;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
    MyPacketEle mypkt;
    int ret = -1;

    SDL_LockMutex(q->mutex);
    for (;;){
        if(av_fifo_read(q->pkts, &mypkt, 1)>=0){
            q->nb_packets--;
            q->size -= mypkt.pkt->size + sizeof(mypkt);
            q->duration -= mypkt.pkt->duration;
            av_packet_move_ref(pkt, mypkt.pkt);
            av_packet_free(&mypkt.pkt);
            ret = 1;
            break;
        }else if(!block){
            ret = 0;
            break;
        }else{
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    
    return ret;
}

static void packet_queue_flush(PacketQueue *q)
{
    MyPacketEle mypkt;
    SDL_LockMutex(q->mutex);

    while (av_fifo_read(q->pkts, &mypkt, 1) > 0){
        av_packet_free(&mypkt.pkt);
    }
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;

    SDL_UnlockMutex(q->mutex);
}

static void packet_queue_destroy(PacketQueue *q)
{
    packet_queue_flush(q);
    av_fifo_freep2(&q->pkts);
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond); 
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    AVPacket *pkt1;
    int ret = -1;

    
    
    SDL_UnlockMutex(q->mutex);

    pkt1 = av_packet_alloc();
    if(!pkt1){
        av_packet_unref(pkt);
        return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    SDL_LockMutex(q->mutex);

    ret = packet_queue_put_priv(q, pkt1);

    SDL_UnlockMutex(q->mutex);

    if(ret < 0){
        av_packet_free(&pkt1);
    }
    return ret;
}

static void render(VideoState *is)
{

    SDL_UpdateYUVTexture(is->texture, NULL,
                         is->vFrame->data[0], is->vFrame->linesize[0] ,
                         is->vFrame->data[1], is->vFrame->linesize[1],
                         is->vFrame->data[2], is->vFrame->linesize[2]);
    
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, is->texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    int frameRate = is->fmtCtx->streams[is->vIdx]->r_frame_rate.num/is->fmtCtx->streams[is->vIdx]->r_frame_rate.den;
    SDL_Delay((Uint32)(ONESECOND/frameRate));
}

static int decode_raw(VideoState *is)
{
    int ret = -1;

    //send packet to decoder
    ret = avcodec_send_packet(is->vCtx, is->vPkt);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
        goto end;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(is->vCtx, is->vFrame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            goto end;
        }else if(ret < 0){
            ret = -1;
            goto end;
        }
        char buffer[1024];
        static int frameNumber = 0;
        char *fileName = "/Users/jacklau/Documents/Programs/C_text/ffmpeg/resource/test/out_raw";
        snprintf(buffer, sizeof(buffer), "%s-%d.pgm", fileName, frameNumber++);
        //printf("frame number: %s\n", frameNumber);
        save_pgm(is->vFrame->data[0], is->vFrame->linesize[0], is->vFrame->width, is->vFrame->height, buffer);
        render(is);
        if(frameNumber > 10) exit(-1);
    }
    

end:
    return ret;
    
}

static int decode(void *arg)
{
    VideoState *is = arg;

    int ret = -1;

    //char buffer[1024];
    SDL_LockMutex(videoMutex);
    while (1)
    {
        
        SDL_CondWait(videoCond, videoMutex);
        
        if(frame_decoded == 1)
        {
            //send packet to decoder
            ret = avcodec_send_packet(is->vCtx, is->vPkt);
            if(ret < 0){
                av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
                break;
            }

            while (ret >= 0)
            {
                ret = avcodec_receive_frame(is->vCtx, is->vFrame);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                    //ret = 0;
                    break;
                }else if(ret < 0){
                    ret = -1;
                    break;
                }
                // char buffer[1024];
                // static int frameNumber = 0;
                // char *fileName = "/Users/jacklau/Documents/Programs/C_text/ffmpeg/resource/test/out_test";
                // snprintf(buffer, sizeof(buffer), "%s-%d.pgm", fileName, frameNumber++);
                // //printf("frame number: %s\n", frameNumber);
                // save_pgm(is->vFrame->data[0], is->vFrame->linesize[0], is->vFrame->width, is->vFrame->height, buffer);
                render(is);
                // if(frameNumber > 10) exit(-1);
                
                av_frame_unref(is->vFrame);
                

            }
            frame_decoded = 0;
        }
    }
    
    SDL_UnlockMutex(videoMutex);
    
    return ret;
}

static int audio_decode_frame(VideoState *is)
{
    int ret = -1;
    int len2 = 0;

    int data_size = 0;
    AVPacket *pkt = av_packet_alloc();
    for(;;){
        if(packet_queue_get(&is->audioQueue, pkt, 1)<0){
            return -1;
        }

        ret = avcodec_send_packet(is->aCtx, pkt);
        if(ret < 0){
            av_log(is->aCtx, AV_LOG_ERROR, "Failed to send pkt to audio decoder!\n");
            goto end;
        }
        while (ret >= 0)
        {
            ret = avcodec_receive_frame(is->aCtx, is->aFrame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            }else if(ret < 0){
                av_log(is->aCtx, AV_LOG_ERROR, "Failed to receive frame from audio decoder!\n");
                goto end;
            }

            
            //re-sampling
            if(!is->swr_ctx){
                AVChannelLayout in_ch_layout, out_ch_layout;
                av_channel_layout_copy(&in_ch_layout, &is->aCtx->ch_layout);
                av_channel_layout_copy(&out_ch_layout, &in_ch_layout);
                if(is->aCtx->sample_fmt != AV_SAMPLE_FMT_S16){
                    swr_alloc_set_opts2(&is->swr_ctx, 
                                    &out_ch_layout, 
                                    AV_SAMPLE_FMT_S16,
                                    is->aCtx->sample_rate,
                                    &in_ch_layout,
                                    is->aCtx->sample_fmt,
                                    is->aCtx->sample_rate,
                                    0,
                                    NULL);

                    swr_init(is->swr_ctx);
                }
            }

            if(is->swr_ctx){
                const uint8_t **in = (const uint8_t **)is->aFrame->extended_data;
                int in_count = is->aFrame->nb_samples;
                uint8_t **out = &is->audio_buf;
                int out_count = is->aFrame->nb_samples + 512;

                int out_size = av_samples_get_buffer_size(NULL, is->aFrame->ch_layout.nb_channels, out_count, AV_SAMPLE_FMT_S16, 0);
                av_fast_malloc(&is->audio_buf, &is->audio_buf_size, out_size);

                len2 = swr_convert(is->swr_ctx,
                            out,
                            out_count,
                            in,
                            in_count);

                //data_size = len2 * is->aFrame->ch_layout.nb_channels*av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
                data_size = len2 * is->fmtCtx->streams[is->aIdx]->codecpar->ch_layout.nb_channels * 2; 
                //data_size = len2 * 2 * 2; 
            }else{
                is->audio_buf = is->vFrame->data[0];
                data_size = av_samples_get_buffer_size(NULL, is->aFrame->ch_layout.nb_channels, is->aFrame->nb_samples, is->aFrame->format, 1);
            }

            av_packet_unref(pkt);
            av_frame_unref(is->aFrame);

            return data_size;

        }
        
    }
end:
    return ret;

}

static void sdl_audio_callback(void *userdata, Uint8 *stream, int len)
{
    int len1 = 0;
    int audio_size = 0;
    VideoState *is = (VideoState*)userdata;


    if (len > 0){
        if(is->audio_buf_index >= is->audio_buf_size){
            audio_size = audio_decode_frame(is);
            if(audio_size < 0){
                is->audio_buf_size = AUDIO_BUFFER_SIZE;
                is->audio_buf = NULL;
            }else {
                is->audio_buf_size = audio_size;
            }
            is->audio_buf_index = 0;
        }
    }
    len1 = is->audio_buf_size - is->audio_buf_index;
    if(len1 > len){
        len1 = len;
    }
    if(is->audio_buf){
        memcpy(stream, (uint8_t*)(is->audio_buf + is->audio_buf_index), len1);
    }else{
        memset(stream, 0, len1);
    }

    len -= len1;
    stream += len1;
    is->audio_buf_index += len1;
}


int main(int argc, char *argv[])
{

    int ret = -1;
    //int vIdx = -1, aIdx = -1;
    //AVFormatContext *fmtCtx = NULL;
    AVStream *aInStream = NULL;
    AVStream *vInStream = NULL;
    
    const AVCodec *aDecodec = NULL;
    const AVCodec *vDecodec = NULL;
    
    AVCodecContext *aCtx = NULL;
    AVCodecContext *vCtx = NULL;

    SDL_Texture *texture = NULL;
    SDL_Event event;

    Uint32 pixformat = 0;

    int video_height = 0;
    int video_width = 0;

    AVPacket *aPkt = NULL;
    AVFrame *aFrame = NULL;

    AVPacket *vPkt = NULL;
    AVFrame *vFrame = NULL;

    AVPacket *pkt = NULL;

    VideoState *is = NULL; 

    SDL_AudioSpec wanted_spec, spec;
    
    // set the mutex and cond
    // pthread_t decode_tid, render_tid;
    // pthread_mutex_init(&mutex, NULL);
    // pthread_cond_init(&cond, NULL);
    videoMutex = SDL_CreateMutex();
    videoCond = SDL_CreateCond();
    
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

    is->aIdx = -1;
    is->vIdx = -1;
    SDL_Thread *videoThread = SDL_CreateThread(decode, "videoThread", (void *)is);
    //pthread_create(&decode_tid, NULL, decode, (void *)is);

    //init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)){
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
    if( (ret = avformat_open_input(&is->fmtCtx, src, NULL, NULL)) < 0 ){
        av_log(NULL, AV_LOG_ERROR, " %s \n", av_err2str(ret));
        goto end;
    }
    if((ret = avformat_find_stream_info(is->fmtCtx, NULL)) < 0){
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        goto end;
    }
    //find the best stream
    for(int i = 0; i < is->fmtCtx->nb_streams; i++){
        if(is->fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && is->vIdx < 0){
            is->vIdx = i;
        }
        if(is->fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && is->aIdx < 0){
            is->aIdx = i;
        }
        if(is->vIdx > -1 && is->aIdx > -1){
            break;
        }
    }
    if(is->vIdx == -1){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find video stream!\n");
        goto end;
    }
    if(is->aIdx == -1){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find audio stream!\n");
        goto end;
    }
    aInStream = is->fmtCtx->streams[is->aIdx];
    vInStream = is->fmtCtx->streams[is->vIdx];

    
    //get decodec by codec_id from stream info
    vDecodec = avcodec_find_decoder(vInStream->codecpar->codec_id);
    if(!vDecodec){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: libx264 \n");
        goto end;
    }
    //init decoder context
    vCtx = avcodec_alloc_context3(vDecodec);
    if(!vCtx){
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        goto end;
    }
    //copy parameters 
    ret = avcodec_parameters_to_context(vCtx, vInStream->codecpar);
    if(ret < 0){
        av_log(vCtx, AV_LOG_ERROR, "Couldn't copy codecpar to codecContext");
        goto end;
    }
    //bind decoder and decoder context
    ret = avcodec_open2(vCtx, vDecodec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        goto end;
    }
    //create texture for render 
    video_width = vCtx->width;
    video_height = vCtx->height;
    pixformat = SDL_PIXELFORMAT_IYUV;
    texture = SDL_CreateTexture(renderer, pixformat, SDL_TEXTUREACCESS_STREAMING, video_width, video_height);

    
   
    
    //get decodec by codec_id from stream info
    aDecodec = avcodec_find_decoder(aInStream->codecpar->codec_id);
    if(!aDecodec){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: libx264 \n");
        goto end;
    }
    //init decoder context
    aCtx = avcodec_alloc_context3(aDecodec);
    if(!aCtx){
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        goto end;
    }
    //copy parameters 
    ret = avcodec_parameters_to_context(aCtx, aInStream->codecpar);
    if(ret < 0){
        av_log(aCtx, AV_LOG_ERROR, "Couldn't copy codecpar to codecContext");
        goto end;
    }
    //bind decoder and decoder context
    ret = avcodec_open2(aCtx, aDecodec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        goto end;
    }
    //init
    packet_queue_init(&is->audioQueue);

    aPkt = av_packet_alloc();
    aFrame = av_frame_alloc();

    vPkt = av_packet_alloc();
    vFrame = av_frame_alloc();

    pkt = av_packet_alloc();

    //init VideoState
    is->texture = texture;
    is->aCtx = aCtx;
    is->aPkt = aPkt;
    is->aFrame = aFrame;
    is->vCtx = vCtx;
    is->vPkt = vPkt;
    is->vFrame = vFrame;

    //set the parameters for audio device
    wanted_spec.freq = aCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = aCtx->ch_layout.nb_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = AUDIO_BUFFER_SIZE;
    wanted_spec.callback = sdl_audio_callback;
    wanted_spec.userdata = (void*)is;

    if(SDL_OpenAudio(&wanted_spec, &spec)<0){
        av_log(NULL, AV_LOG_ERROR, "Failed to open audio device!\n");
        goto end;
    }
    SDL_PauseAudio(0);
    //decode video
    while(av_read_frame(is->fmtCtx, pkt) >= 0){
        if(pkt->stream_index == is->vIdx ){
            av_packet_move_ref(is->vPkt, pkt);
            // decode and render
            // decode_raw(is);
            frame_decoded = 1;
            SDL_LockMutex(videoMutex);
            SDL_CondSignal(videoCond);
            SDL_UnlockMutex(videoMutex);
        }else if(pkt->stream_index == is->aIdx){
            packet_queue_put(&is->audioQueue, pkt);
        }else{
            av_packet_unref(pkt);
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
    
    }
    is->vPkt = NULL;
    frame_decoded = 1;
    SDL_LockMutex(videoMutex);
    SDL_CondSignal(videoCond);
    SDL_UnlockMutex(videoMutex);
    //decode(is);

    SDL_WaitThread(videoThread, NULL);

quit:
    ret = 0;
end:
    // pthread_mutex_destroy(&mutex);
    // pthread_cond_destroy(&cond);
    SDL_DestroyCond(videoCond);
    SDL_DestroyMutex(videoMutex);
    if(vFrame){
        av_frame_free(&vFrame);
    }
    if(aFrame){
        av_frame_free(&aFrame);
    }
    if (pkt){
        av_packet_free(&pkt);
    }
    if (aPkt){
        av_packet_free(&aPkt);
    }
    if (vPkt){
        av_packet_free(&vPkt);
    }
    if(aCtx){
        avcodec_free_context(&aCtx);
    }
    if(vCtx){
        avcodec_free_context(&vCtx);
    }
    if(is->fmtCtx){
        avformat_close_input(&is->fmtCtx);
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
    if(is){
        av_free(is);
    }

    SDL_Quit();    
    return ret;
}