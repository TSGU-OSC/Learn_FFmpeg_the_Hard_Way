/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about transcoding and remuxing video through ffmpeg API
 * 
 * FFmpeg version 5.1.4 
 * Tested on MacOS 14.1.2, compiled with clang 14.0.3
 */

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>

#define ENCODE_BIT_RATE 500000
#define ERROR -1

typedef struct StreamContext
{
    AVFormatContext *fmtCtx;
    char *filename;
    AVStream *videoStream;
    const AVCodec *videoCodec;
    AVCodecContext *videoCodecCtx;
    const AVCodec *audioCodec;
    AVCodecContext *audioCodecCtx;
    AVPacket *pkt;
    AVFrame *frame;
}StreamContext;

int copyFrame(AVFrame* oldFrame, AVFrame* newFrame)
{
	int response;
	newFrame->pts = oldFrame->pts;
	newFrame->format = oldFrame->format;
	newFrame->width = oldFrame->width;
	newFrame->height = oldFrame->height;
	//newFrame->channels = oldFrame->channels;
	//newFrame->channel_layout = oldFrame->channel_layout;
    newFrame->ch_layout = oldFrame->ch_layout;
	newFrame->nb_samples = oldFrame->nb_samples;
	response = av_frame_get_buffer(newFrame, 32);
	if (response != 0)
	{
		return ERROR;
	}
	response = av_frame_copy(newFrame, oldFrame);
	if (response >= 0)
	{
		return ERROR;
	}
	response = av_frame_copy_props(newFrame, oldFrame);
	if (response == 0)
	{
		return ERROR;
	}
	return 0;
}

static int encode(AVStream *inStream, StreamContext *encoder)
{
    int ret = -1;

    //send frame to encoder
    ret = avcodec_send_frame(encoder->videoCodecCtx, encoder->frame);
    if(ret < 0){
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        ret = av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to encoder! %s\n", errbuf);
        goto end;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(encoder->videoCodecCtx, encoder->pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){

            return -1;
        }
        encoder->pkt->stream_index = encoder->videoStream->index;
        encoder->pkt->duration = encoder->videoStream->time_base.den / encoder->videoStream->time_base.num / inStream->avg_frame_rate.num * inStream->avg_frame_rate.den;

        av_packet_rescale_ts(encoder->pkt, inStream->time_base, encoder->videoStream->time_base);


        ret = av_interleaved_write_frame(encoder->fmtCtx, encoder->pkt);
        if(ret < 0)
        {
            fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
        }

        av_packet_unref(encoder->pkt);
    }
    

end:
    return 0;
}

static int transcode(StreamContext *decoder, StreamContext *encoder)
{
    int ret = -1;

    char buffer[1024];
    //send packet to decoder
    ret = avcodec_send_packet(decoder->videoCodecCtx, decoder->pkt);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
        goto end;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(decoder->videoCodecCtx, decoder->frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){

            return -1;
        }

        copyFrame(decoder->frame, encoder->frame);

        encode(decoder->videoStream, encoder);

        if (decoder->pkt)
            av_packet_unref(decoder->pkt);

        av_frame_unref(decoder->frame);
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

    StreamContext *decoder = malloc(sizeof(StreamContext));
    StreamContext *encoder = malloc(sizeof(StreamContext));

    av_log_set_level(AV_LOG_DEBUG);
    if(argc < 3){
        av_log(NULL, AV_LOG_ERROR, "the arguments must be more than 3!\n");
    }

    src = argv[1];
    dst = argv[2];
    
    //open the multimedia file
    if( (ret = avformat_open_input(&decoder->fmtCtx, src, NULL, NULL)) < 0 ){
        av_log(NULL, AV_LOG_ERROR, " %s \n", av_err2str(ret));
        exit(-1);
    }
    
    //find the video stream from container
    if((idx = av_find_best_stream(decoder->fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0){
        av_log(decoder->fmtCtx, AV_LOG_ERROR, "There is no audio stream!\n");
        goto end;
    }

    decoder->videoStream = decoder->fmtCtx->streams[idx];

    //find the decoder
    //codec = avcodec_find_encoder_by_name(codecID);
    //find the decoder by ID

    decoder->videoCodec = avcodec_find_decoder(decoder->videoStream->codecpar->codec_id);
    if(!decoder->videoCodec){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: libx264 \n");
        goto end;
    }

    //init decoder context
    decoder->videoCodecCtx = avcodec_alloc_context3(decoder->videoCodec);
    if(!decoder->videoCodec){
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        goto end;
    }

    avcodec_parameters_to_context(decoder->videoCodecCtx, decoder->videoStream->codecpar);

    //bind decoder and decoder context
    ret = avcodec_open2(decoder->videoCodecCtx, decoder->videoCodec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        goto end;
    }

    //create AVFrame
    decoder->frame = av_frame_alloc();
    if(!decoder->frame){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        goto end;
    }

   

    //create AVPacket
    decoder->pkt = av_packet_alloc();
    if(!decoder->pkt){
        av_log(NULL, AV_LOG_ERROR, "NO Memory!\n");
        goto end;
    }

    /**
     * set the output file parameters
     */
    ret = avformat_alloc_output_context2(&encoder->fmtCtx, NULL, NULL, dst);
    if (!encoder->fmtCtx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        goto end;
    }

    

   //find the encodec by ID
    encoder->videoCodec = avcodec_find_encoder(decoder->videoCodecCtx->codec_id);
    if(!encoder->videoCodec){
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: \n");
        goto end;
    }

    
    
    //init codec context
    encoder->videoCodecCtx = avcodec_alloc_context3(encoder->videoCodec);
    if(!encoder->videoCodecCtx){
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        goto end;
    }

    if(decoder->videoCodecCtx->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        encoder->videoCodecCtx->height = decoder->videoCodecCtx->height;
        encoder->videoCodecCtx->width = decoder->videoCodecCtx->width;
        encoder->videoCodecCtx->bit_rate = ENCODE_BIT_RATE;
        encoder->videoCodecCtx->sample_aspect_ratio = decoder->videoCodecCtx->sample_aspect_ratio;
        //the AVCodecContext don't have framerate
        //outCodecCtx->time_base = av_inv_q(inCodecCtx->framerate);
        encoder->videoCodecCtx->time_base = (AVRational){1, 60};
        encoder->videoCodecCtx->framerate = (AVRational){60, 1};
        // if(inCodecCtx->pix_fmt)
        //     outCodecCtx->pix_fmt = inCodecCtx->pix_fmt;
        // else
        encoder->videoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        encoder->videoCodecCtx->max_b_frames = 0;
    }

    //bind codec and codec context
    ret = avcodec_open2(encoder->videoCodecCtx, encoder->videoCodec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        goto end;
    }
   
    //create AVFrame
    encoder->frame = av_frame_alloc();
    if(!encoder->frame){
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        goto end;
    }

    encoder->frame->width = encoder->videoCodecCtx->width;
    encoder->frame->height = encoder->videoCodecCtx->height;
    encoder->frame->format = encoder->videoCodecCtx->pix_fmt;

    ret = av_frame_get_buffer(encoder->frame, 0);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't allocate the video frame\n");
        goto end;
    }

    //create AVPacket
    encoder->pkt = av_packet_alloc();
    if(!encoder->pkt){
        av_log(NULL, AV_LOG_ERROR, "NO Memory!\n");
        goto end;
    }

    encoder->videoStream = avformat_new_stream(encoder->fmtCtx, NULL);
    if (!encoder->videoStream) {
        av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
        goto end;
    }
    encoder->videoStream->r_frame_rate = (AVRational){60, 1}; // For setting real frame rate
    encoder->videoStream->avg_frame_rate = (AVRational){60, 1}; // For setting average frame rate
    //the input file's time_base is wrong
    encoder->videoStream->time_base = encoder->videoCodecCtx->time_base;

    ret = avcodec_parameters_from_context(encoder->videoStream->codecpar, encoder->videoCodecCtx);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #\n");
        return ret;
    }

    

    // oFmtCtx->oformat = av_guess_format(NULL, dst, NULL);
    // if(!oFmtCtx->oformat)
    // {
    //     av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
    // }

    //binding
    ret = avio_open2(&encoder->fmtCtx->pb, dst, AVIO_FLAG_WRITE, NULL, NULL);
    if(ret < 0){
        av_log(encoder->fmtCtx, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto end;
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(encoder->fmtCtx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        goto end;
    }

    

    //read video data from multimedia files to write into destination file
    while(av_read_frame(decoder->fmtCtx, decoder->pkt) >= 0){
        if(decoder->pkt->stream_index == idx ){
            transcode(decoder, encoder);
            //encode(oFmtCtx, outCodecCtx, outFrame, outPkt, inStream, outStream);
        }
    }
    decoder->pkt = NULL;
    //write the buffered frame
    transcode(decoder, encoder);
    //encode(oFmtCtx, outCodecCtx, outFrame, outPkt, inStream, outStream);

    av_write_trailer(encoder->fmtCtx);

    //free memory
end:
    if(decoder->fmtCtx){
        avformat_close_input(&decoder->fmtCtx);
        decoder->fmtCtx = NULL;
    }
    if(decoder->videoCodecCtx){
        avcodec_free_context(&decoder->videoCodecCtx);
        decoder->videoCodecCtx = NULL;
    }
    if (decoder->frame){
        av_frame_free(&decoder->frame);
        decoder->frame = NULL;
    }
    if (decoder->pkt){
        av_packet_free(&decoder->pkt);
        decoder->pkt = NULL;
    }

    if(encoder->fmtCtx && !(encoder->fmtCtx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder->fmtCtx->pb);
    if(encoder->fmtCtx){
        avformat_free_context(encoder->fmtCtx);
        encoder->fmtCtx = NULL;
    }

    return 0;
}