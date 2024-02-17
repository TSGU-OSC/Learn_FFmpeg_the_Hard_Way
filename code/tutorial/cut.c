/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about cuting the duration of multimedia file from a container into a new container through ffmpeg API
 * 
 * FFmpeg version 5.0.3 
 * Tested on MacOS 14.1.2, compiled with clang 14.0.3
 */

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

int main(int argc, char *argv[])
{
    int ret = -1;
    int idx = -1;
    int stream_index = 0;
    //deal with arguments
    char *src;
    char *dst;

    double startTime, endTime;

    int *stream_map = NULL;

    AVFormatContext *pFmtCtx = NULL;
    AVFormatContext *oFmtCtx = NULL;

    const AVOutputFormat *outFmt = NULL;

    int64_t *dtsStartTime = NULL;
    int64_t *ptsStartTime = NULL;

    AVPacket pkt;

    av_log_set_level(AV_LOG_DEBUG);
    if(argc < 5){
        av_log(NULL, AV_LOG_ERROR, "the arguments must be more than 5!\n");
    }

    src = argv[1];
    dst = argv[2];
    startTime = atof(argv[3]);
    endTime = atof(argv[4]);
    
    //open the multimedia file
    if( (ret = avformat_open_input(&pFmtCtx, src, NULL, NULL)) < 0 ){
        av_log(NULL, AV_LOG_ERROR, " %s \n", av_err2str(ret));
        exit(-1);
    }


    avformat_alloc_output_context2(&oFmtCtx, NULL, NULL, dst);
    if(!oFmtCtx){
        av_log(NULL, AV_LOG_ERROR, "No Memory\n");
        goto end;
    }

    stream_map = av_calloc(pFmtCtx->nb_streams, sizeof(int));
    if(!stream_map){
        av_log(NULL, AV_LOG_ERROR, "No Memory\n");
        goto end;
    }

    for (int i = 0; i < pFmtCtx->nb_streams; i++)
    {
        AVStream *outStream = NULL;
        AVStream *inStream = pFmtCtx->streams[i];
        AVCodecParameters *inCodecPar = inStream->codecpar;
        if(inCodecPar->codec_type != AVMEDIA_TYPE_AUDIO &&
           inCodecPar->codec_type != AVMEDIA_TYPE_VIDEO &&
           inCodecPar->codec_type != AVMEDIA_TYPE_SUBTITLE){
            stream_map[i] = -1;
            continue;
        }
        stream_map[i] = stream_index++;
        //create a new stream
        outStream = avformat_new_stream(oFmtCtx, NULL);
        if(!outStream){
            av_log(oFmtCtx, AV_LOG_ERROR, "No Memory!\n");
        }

        //set the arguments of output Stream
        avcodec_parameters_copy(outStream->codecpar, inStream->codecpar); 
        outStream->codecpar->codec_tag = 0;
    }
    
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
    //seek the frame we need to find
    ret = av_seek_frame(pFmtCtx, -1, startTime*AV_TIME_BASE, AVSEEK_FLAG_BACKWARD);
    if(ret < 0){
        av_log(oFmtCtx, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto end;
    }
    //set the start time
    dtsStartTime = av_calloc(pFmtCtx->nb_streams, sizeof(int64_t));
    ptsStartTime = av_calloc(pFmtCtx->nb_streams, sizeof(int64_t));

    for(int t = 0; t<pFmtCtx->nb_streams; t++){
        dtsStartTime[t] = -1;
        ptsStartTime[t] = -1;
    }

    //read data from multimedia files to write into destination file
    while(av_read_frame(pFmtCtx, &pkt) >= 0){
        AVStream *inStream, *outStream;
        
        if(dtsStartTime[pkt.stream_index] == -1 && pkt.dts > 0){
            dtsStartTime[pkt.stream_index] = pkt.dts;
        }
        if(ptsStartTime[pkt.stream_index] == -1 && pkt.pts > 0){
            ptsStartTime[pkt.stream_index] = pkt.pts;
        }

        

        pkt.dts = pkt.dts - dtsStartTime[pkt.stream_index];
        pkt.pts = pkt.pts - ptsStartTime[pkt.stream_index];

        if(pkt.dts > pkt.pts){
            pkt.dts = pkt.pts;
        }

        inStream = pFmtCtx->streams[pkt.stream_index];
        //if reach the end time, then break the loop
        if(av_q2d(inStream->time_base) * pkt.pts > endTime){
            av_log(oFmtCtx, AV_LOG_INFO,"success!\n");
            break;
        }
        if(stream_map[stream_index] < 0){
            av_packet_unref(&pkt);
            continue;
        }
        pkt.stream_index = stream_map[pkt.stream_index];

        outStream = oFmtCtx->streams[pkt.stream_index];
        av_packet_rescale_ts(&pkt, inStream->time_base, outStream->time_base);

       
        pkt.pos = -1;
        av_interleaved_write_frame(oFmtCtx, &pkt);
        av_packet_unref(&pkt); 
        
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
    if(stream_map){
        av_free(stream_map);
    }
    if(dtsStartTime){
        av_free(dtsStartTime);
    }
    if(ptsStartTime){
        av_free(ptsStartTime);
    }

    return 0;
}