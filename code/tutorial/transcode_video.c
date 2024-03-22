/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about transcoding video through ffmpeg API
 * 
 * FFmpeg version 5.1.4 
 * Tested on MacOS 14.1.2, compiled with clang 14.0.3
 */

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>

#define ENCODE_BIT_RATE 500000

static int encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *outPkt, FILE *file)
{
    int ret = -1;
    //send frame to encoder
    ret = avcodec_send_frame(ctx, frame);
    if(ret < 0){
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        ret = av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to encoder! %s\n", errbuf);
        goto end;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(ctx, outPkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){

            return -1;
        }

        fwrite(outPkt->data, 1, outPkt->size, file);
        av_packet_unref(outPkt);
    }
    

end:
    return 0;
}

static int decode(AVCodecContext *ctx, AVFrame *frame, AVFrame *outFrame, AVPacket *inPkt, const char *fileName)
{
    int ret = -1;

    char buffer[1024];
    //send packet to decoder
    ret = avcodec_send_packet(ctx, inPkt);
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
        // snprintf(buffer, sizeof(buffer), "%s-%d", fileName, ctx->frame_number);

        // save_pgm(frame->data[0],
        //          frame->linesize[0],
        //          frame->width,
        //          frame->height,
        //          buffer);
        
        
        //encode
        for (size_t i = 0; i < 3; i++)
            outFrame->data[i] = frame->data[i];

        outFrame->pts = frame->pts;

        if (inPkt)
            av_packet_unref(inPkt);
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

    AVPacket *inPkt = NULL;
    AVPacket *outPkt = NULL;

    const AVCodec *inCodec = NULL;
    const AVCodec *outCodec = NULL;
    
    AVCodecContext *inCodecCtx = NULL;
    AVCodecContext *outCodecCtx = NULL;

    AVFrame *inFrame = NULL;
    AVFrame *outFrame = NULL;

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

    inCodec = avcodec_find_decoder(inStream->codecpar->codec_id);
    if(!inCodec){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: libx264 \n");
        goto end;
    }

    //init decoder context
    inCodecCtx = avcodec_alloc_context3(inCodec);
    if(!inCodec){
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        goto end;
    }

    avcodec_parameters_to_context(inCodecCtx, inStream->codecpar);

    //bind decoder and decoder context
    ret = avcodec_open2(inCodecCtx, inCodec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        goto end;
    }

    //create AVFrame
    inFrame = av_frame_alloc();
    if(!inFrame){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        goto end;
    }

   

    //create AVPacket
    inPkt = av_packet_alloc();
    if(!inPkt){
        av_log(NULL, AV_LOG_ERROR, "NO Memory!\n");
        goto end;
    }

    /**
     * set the output file parameters
     */
   //find the encodec by ID
    outCodec = avcodec_find_encoder(inCodecCtx->codec_id);
    if(!outCodec){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: \n");
        goto end;
    }
    
    //init codec context
    outCodecCtx = avcodec_alloc_context3(outCodec);
    if(!outCodecCtx){
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        goto end;
    }

    if(inCodecCtx->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        outCodecCtx->height = inCodecCtx->height;
        outCodecCtx->width = inCodecCtx->width;
        outCodecCtx->bit_rate = ENCODE_BIT_RATE;
        //the AVCodecContext don't have framerate
        //outCodecCtx->time_base = av_inv_q(inCodecCtx->framerate);
        outCodecCtx->time_base = (AVRational){1, 30};
        // if(inCodecCtx->pix_fmt)
        //     outCodecCtx->pix_fmt = inCodecCtx->pix_fmt;
        // else
        outCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    }

    //bind codec and codec context
    ret = avcodec_open2(outCodecCtx, outCodec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        goto end;
    }

    //create output file
    FILE *f = fopen(dst, "wb");
    if(!f){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open file: %s\n", dst);
        goto end;
    }

    //create AVFrame
    outFrame = av_frame_alloc();
    if(!outFrame){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        goto end;
    }

    outFrame->width = outCodecCtx->width;
    outFrame->height = outCodecCtx->height;
    outFrame->format = outCodecCtx->pix_fmt;

    ret = av_frame_get_buffer(outFrame, 0);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't allocate the video frame\n");
        goto end;
    }

    //create AVPacket
    outPkt = av_packet_alloc();
    if(!outPkt){
        av_log(NULL, AV_LOG_ERROR, "NO Memory!\n");
        goto end;
    }
    
    //read video data from multimedia files to write into destination file
    while(av_read_frame(pFmtCtx, inPkt) >= 0){
        if(inPkt->stream_index == idx ){
            decode(inCodecCtx, inFrame, outFrame, inPkt, dst);
            encode(outCodecCtx, outFrame, outPkt, f);
        }
    }
    //write the buffered frame
    decode(inCodecCtx, inFrame, outFrame, NULL, dst);
    encode(outCodecCtx, outFrame, outPkt, f);

    //free memory
end:
    if(pFmtCtx){
        avformat_close_input(&pFmtCtx);
        pFmtCtx = NULL;
    }
    if(inCodecCtx){
        avcodec_free_context(&inCodecCtx);
        inCodecCtx = NULL;
    }
    if (inFrame){
        av_frame_free(&inFrame);
        inFrame = NULL;
    }
    if (inPkt){
        av_packet_free(&inPkt);
        inPkt = NULL;
    }

    return 0;
}