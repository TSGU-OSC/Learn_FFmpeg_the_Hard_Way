/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about remuxing multimedia file from a container into a new container through ffmpeg API
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
    int out_flag = -1;
    enum AVMediaType avMt = AVMEDIA_TYPE_UNKNOWN;
    int out_streams = 0;
    int stream_index = 0;
    //deal with arguments
    char *src;
    char *dst;

    int *stream_map = NULL;

    AVFormatContext *pFmtCtx = NULL;
    AVFormatContext *oFmtCtx = NULL;

    const AVOutputFormat *outFmt = NULL;

    AVStream *inStream = NULL, *outStream = NULL;


    AVPacket *pkt = NULL;

    av_log_set_level(AV_LOG_DEBUG);
    if(argc < 3){
        av_log(NULL, AV_LOG_ERROR, "the arguments must be more than 3!\n");
    }

    src = argv[1];
    dst = argv[2];

    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate AVPacket\n");
        return 1;
    }
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

    //get the output format
    outFmt = av_guess_format(NULL, dst, NULL);
    if(!outFmt){
        av_log(NULL, AV_LOG_ERROR, "No Memory\n");
        goto end;
    }
    oFmtCtx->oformat = outFmt;
    
    if(oFmtCtx->oformat->video_codec != AV_CODEC_ID_NONE)
    {
        out_streams++;
        avMt = AVMEDIA_TYPE_VIDEO;
    }
        
    if(oFmtCtx->oformat->audio_codec != AV_CODEC_ID_NONE)
    {
        out_streams++;
        avMt = AVMEDIA_TYPE_AUDIO;
    }
        
    if(oFmtCtx->oformat->subtitle_codec != AV_CODEC_ID_NONE)
    {
        out_streams++;
        avMt = AVMEDIA_TYPE_SUBTITLE;
    }
    
    if(out_streams < 2)
    {
        out_flag = 1;
    }
        
    stream_map = av_calloc(pFmtCtx->nb_streams, sizeof(int));
    if(!stream_map){
        av_log(NULL, AV_LOG_ERROR, "No Memory\n");
        goto end;
    }



    for (int i = 0; i < pFmtCtx->nb_streams; i++)
    {   
        //outStream = NULL;
        inStream = pFmtCtx->streams[i];
        AVCodecParameters *inCodecPar = inStream->codecpar;
        if(inCodecPar->codec_type != AVMEDIA_TYPE_AUDIO &&
           inCodecPar->codec_type != AVMEDIA_TYPE_VIDEO &&
           inCodecPar->codec_type != AVMEDIA_TYPE_SUBTITLE
           )
        {

            stream_map[i] = -1;
            continue;
        }

        if (inCodecPar->codec_type != avMt && out_flag > 0 )
        {
            stream_map[i] = -1;
            continue;
        }
        

        stream_map[i] = stream_index++;
        

        idx = i;

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

    if(oFmtCtx->oformat->audio_codec == AV_CODEC_ID_NONE && stream_index < 2)
    {
        while(av_read_frame(pFmtCtx, pkt) >= 0){
            if(pkt->stream_index == idx ){
                pkt->pts = av_rescale_q_rnd(pkt->pts, inStream->time_base, outStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                //the dts data maybe different with the the pts if the file is video
                pkt->dts = av_rescale_q_rnd(pkt->dts, inStream->time_base, outStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->duration = av_rescale_q(pkt->duration, inStream->time_base, outStream->time_base);
                pkt->stream_index = 0;
                pkt->pos = -1;
                av_interleaved_write_frame(oFmtCtx, pkt);
                av_packet_unref(pkt); 
            }
        }
    }else if(oFmtCtx->oformat->video_codec == AV_CODEC_ID_NONE && stream_index < 2)
    {
        //read audio data from multimedia files to write into destination file
        while(av_read_frame(pFmtCtx, pkt) >= 0){
            if(pkt->stream_index == idx ){
                pkt->pts = av_rescale_q_rnd(pkt->pts, inStream->time_base, outStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                //the dts data is same as the the pts if the file is audio
                pkt->dts = pkt->pts;
                pkt->duration = av_rescale_q(pkt->duration, inStream->time_base, outStream->time_base);
                pkt->stream_index = 0;
                pkt->pos = -1;
                av_interleaved_write_frame(oFmtCtx, pkt);
                av_packet_unref(pkt); 
            }
        }
    }else
    {
        //read data from multimedia files to write into destination file
        while(av_read_frame(pFmtCtx, pkt) >= 0){
            
            AVStream *inStreamTem, *outStreamTem;
            
            inStreamTem = pFmtCtx->streams[pkt->stream_index];
            if(stream_map[stream_index] < 0){
                av_packet_unref(pkt);
                continue;
            }
            pkt->stream_index = stream_map[pkt->stream_index];

            outStreamTem = oFmtCtx->streams[pkt->stream_index];
            av_packet_rescale_ts(pkt, inStreamTem->time_base, outStreamTem->time_base);

        
                pkt->pos = -1;
                av_interleaved_write_frame(oFmtCtx, pkt);
                av_packet_unref(pkt); 
            
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
        avformat_free_context(oFmtCtx);
        oFmtCtx = NULL;
    }
    if(stream_map){
        av_free(stream_map);
    }
    if (pkt)
    {
        av_packet_free(&pkt);
    }
    

    return 0;
}
