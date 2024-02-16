/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about extrating audio stream from a container into a new container through ffmpeg API
 * 
 * FFmpeg version 5.0.3 
 * Tested on Ubuntu 22.04, compiled with GCC 11.4.0
 */

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

int main(int argc, char *argv[])
{
    int ret = -1;
    int idx = -1;
    //deal with arguments
    char *src;
    char *dst;

    AVFormatContext *pFmtCtx = NULL;
    AVFormatContext *oFmtCtx = NULL;

    const AVOutputFormat *outFmt = NULL;
    AVStream *inStream = NULL;
    AVStream *outStream = NULL;

    AVPacket pkt;

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

    //find the audio stream from container
    if((idx = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0)) < 0){
        av_log(pFmtCtx, AV_LOG_ERROR, "There is no audio stream!\n");
        goto end;
    }

    //open the context of destination file
    if((oFmtCtx = avformat_alloc_context()) < 0){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        goto end;
    }

    outFmt = av_guess_format(NULL, dst, NULL);
    oFmtCtx->oformat = outFmt; 

    //create a audio stream
    outStream = avformat_new_stream(oFmtCtx, NULL);

    //set the arguments of output Stream
    inStream = pFmtCtx->streams[idx];
    avcodec_parameters_copy(outStream->codecpar, inStream->codecpar); 
    outStream->codecpar->codec_tag = 0;

    //binding
    ret = avio_open2(&oFmtCtx->pb, dst, AVIO_FLAG_WRITE, NULL, NULL);
    if(ret < 0){
        av_log(oFmtCtx, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto end;
    }

    //write the head file of multimedia to destination file
    ret = avformat_write_header(oFmtCtx, NULL);
    if(ret < 0){
        av_log(oFmtCtx, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto end;
    }

    //read audio data from multimedia files to write into destination file
    while(av_read_frame(pFmtCtx, &pkt) >= 0){
        if(pkt.stream_index == idx ){
            pkt.pts = av_rescale_q_rnd(pkt.pts, inStream->time_base, outStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.dts = pkt.pts;
            pkt.duration = av_rescale_q(pkt.duration, inStream->time_base, outStream->time_base);
            pkt.stream_index = 0;
            pkt.pos = -1;
            av_interleaved_write_frame(oFmtCtx, &pkt);
            av_packet_unref(&pkt); 
        }
    }

    //write end of file
    av_write_trailer(oFmtCtx);

    //free memory
end:
    if(pFmtCtx){
        avformat_close_input(&pFmtCtx);
        pFmtCtx = NULL;
    }
    if(oFmtCtx->pb){
        avio_close(oFmtCtx->pb);
    }
    if(oFmtCtx){
        avformat_close_input(&oFmtCtx);
        oFmtCtx = NULL;
    }

    return 0;
}