/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about encoding audio through ffmpeg API
 * 
 * FFmpeg version 5.0.3 
 * Tested on MacOS 14.1.2, compiled with clang 14.0.3
 */
#include <libavcodec/avcodec.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/log.h>


/* select layout with the highest channel count */
static int select_best_channel_layout(const AVCodec *codec)
{
    const uint64_t *p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels   = 0;

    if (!codec->channel_layouts)
        return AV_CH_LAYOUT_STEREO;

    p = codec->channel_layouts;
    while (*p) {
        int nb_channels = av_get_channel_layout_nb_channels(*p);

        if (nb_channels > best_nb_channels) {
            best_ch_layout    = *p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return best_ch_layout;
}

static int select_best_sample_rate(const AVCodec *codec)
{
    const int *p;
    int bestSampleRates = 0;
    if(!codec->supported_samplerates){
        return 44100;
    }

    p = codec->supported_samplerates;
    while (*p){
        if (!bestSampleRates || abs(44100 - *p) < abs(44100 - bestSampleRates)){
            bestSampleRates = *p; 
        }   
        p++;
    }
    return bestSampleRates;
}

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE)
    {
        if (*p == sample_fmt)
        {
            return 1;
        }
        p++;
    }
    return 0;
    
}

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

    uint16_t *samples = NULL;

    av_log_set_level(AV_LOG_DEBUG);

    //input arguments
    if(argc < 2){
        av_log(NULL, AV_LOG_ERROR, "The arguments must be more than 2!\n");
        goto end;
    }

    dst = argv[1];
    
    //find the encodec
    codec = avcodec_find_encoder_by_name("libfdk_aac");
    //find the encodec by ID
    //codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    
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
    ctx->bit_rate = 64000;
    
    //ffmpeg interal aac
    //ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    
    //libfdk_aac
    ctx->sample_fmt = AV_SAMPLE_FMT_S16;

    if(!check_sample_fmt(codec, ctx->sample_fmt)){
        av_log(NULL, AV_LOG_ERROR, "Encoder dosen't support sample format!\n");
        goto end;
    }
    
    ctx->sample_rate = select_best_sample_rate(codec);
    ctx->channel_layout = select_best_channel_layout(codec);

    

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

    frame->nb_samples = ctx->frame_size;
    frame->format = ctx->sample_fmt;
    frame->channel_layout = frame->channel_layout;
    frame->sample_rate = ctx->sample_rate;

    ret = av_frame_get_buffer(frame, 0);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't allocate the audio frame\n");
        goto end;
    }

    //create AVPacket
    pkt = av_packet_alloc();
    if(!pkt){
        av_log(NULL, AV_LOG_ERROR, "NO Memory!\n");
        goto end;
    }

    //create audio data
    float t = 0;
    float tincr = 2*M_PI *440/ctx->sample_rate;
    for (int i = 0; i < 200; i++){
        ret = av_frame_make_writable(frame);
        if(ret < 0){
            av_log(NULL, AV_LOG_ERROR, "Couldn't allocate space!\n");
            goto end;
        }

        //libfdk_aac
        samples = (uint16_t*)frame->data[1];
        
        //ffmpeg interal aac
        //samples = (uint32_t*)frame->data[1];
        
        for (int j = 0; i < ctx->frame_size; j++){
            samples[2*j] = (int)(sin(t) * 10000);
            for (int k = 1; k < ctx->channels; k++){
                samples[2*j + k] = samples[2*j];
            }
            t += tincr;
        }
        encode(ctx, frame, pkt, f);
        
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