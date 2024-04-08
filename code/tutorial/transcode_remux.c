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

    int videoIdx;
    AVStream *videoStream;
    const AVCodec *videoCodec;
    AVCodecContext *videoCodecCtx;
    
    int audioIdx;
    AVStream *audioStream;
    const AVCodec *audioCodec;
    AVCodecContext *audioCodecCtx;
    
    AVPacket *pkt;
    AVFrame *frame;
}StreamContext;

static int open_Media(StreamContext *decoder, StreamContext *encoder)
{
    int ret = -1;
    //open the multimedia file
    if( (ret = avformat_open_input(&decoder->fmtCtx, decoder->filename, NULL, NULL)) < 0 )
    {
        av_log(NULL, AV_LOG_ERROR, " %s \n", av_err2str(ret));
        return -1;
    }

    ret = avformat_alloc_output_context2(&encoder->fmtCtx, NULL, NULL, encoder->filename);
    if (!encoder->fmtCtx) 
    {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return -1;
    } 

    return 0;
}

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

static int encode_Video(AVStream *inStream, StreamContext *encoder)
{
    int ret = -1;

    //send frame to encoder
    ret = avcodec_send_frame(encoder->videoCodecCtx, encoder->frame);
    if(ret < 0)
    {
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

static int transcode_Video(StreamContext *decoder, StreamContext *encoder)
{
    int ret = -1;

    char buffer[1024];
    //send packet to decoder
    ret = avcodec_send_packet(decoder->videoCodecCtx, decoder->pkt);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to send frame to decoder!\n");
        goto end;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(decoder->videoCodecCtx, decoder->frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }else if(ret < 0)
        {

            return -1;
        }

        copyFrame(decoder->frame, encoder->frame);

        encode_Video(decoder->videoStream, encoder);

        if (decoder->pkt)
        {
            av_packet_unref(decoder->pkt);
        }
            
        av_frame_unref(decoder->frame);
    }
    

end:
    return 0;
}

static int prepare_Decoder(StreamContext *decoder)
{
    int ret = -1;

    
    for (int i = 0; i < decoder->fmtCtx->nb_streams; i++)
    {
        if (decoder->fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            decoder->videoStream = decoder->fmtCtx->streams[i];
            decoder->videoIdx = i;
        }else if(decoder->fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            decoder->audioStream = decoder->fmtCtx->streams[i];
            decoder->audioIdx = i;
        }
        
    }
    

    //find the decoder
    //codec = avcodec_find_encoder_by_name(codecID);
    //find the decoder by ID

    decoder->videoCodec = avcodec_find_decoder(decoder->videoStream->codecpar->codec_id);
    if(!decoder->videoCodec)
    {
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: %s \n", avcodec_get_name(decoder->videoStream->codecpar->codec_id));
        //return -1;
    }

    decoder->audioCodec = avcodec_find_decoder(decoder->audioStream->codecpar->codec_id);
    if(!decoder->audioCodec)
    {
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: %s \n", avcodec_get_name(decoder->audioStream->codecpar->codec_id));
    }

    //init decoder context
    decoder->videoCodecCtx = avcodec_alloc_context3(decoder->videoCodec);
    if(!decoder->videoCodec)
    {
        av_log(decoder->videoCodecCtx, AV_LOG_ERROR, "No memory!\n");
        //return -1;
    }

    decoder->audioCodecCtx = avcodec_alloc_context3(decoder->audioCodec);
    if(!decoder->audioCodec)
    {
        av_log(decoder->audioCodecCtx, AV_LOG_ERROR, "No Memory!");
    }

    avcodec_parameters_to_context(decoder->videoCodecCtx, decoder->videoStream->codecpar);
    avcodec_parameters_to_context(decoder->audioCodecCtx, decoder->audioStream->codecpar);
    //bind decoder and decoder context
    ret = avcodec_open2(decoder->videoCodecCtx, decoder->videoCodec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        //return -1;
    }

    ret = avcodec_open2(decoder->audioCodecCtx, decoder->audioCodec, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        //return -1;
    }

    //create AVFrame
    decoder->frame = av_frame_alloc();
    if(!decoder->frame)
    {
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        return -1;
    }

    //create AVPacket
    decoder->pkt = av_packet_alloc();
    if(!decoder->pkt)
    {
        av_log(NULL, AV_LOG_ERROR, "NO Memory!\n");
        return -1;
    }




    return 0;
}

static int prepare_Encoder_Video(StreamContext *decoder, StreamContext *encoder)
{
    int ret = -1;

    /**
     * set the output file parameters
     */
    

   //find the encodec by ID
    encoder->videoCodec = avcodec_find_encoder(decoder->videoCodecCtx->codec_id);
    if(!encoder->videoCodec)
    {
        av_log(NULL, AV_LOG_ERROR, "Couldn't find codec: \n");
        return -1;
    }

    //init codec context
    encoder->videoCodecCtx = avcodec_alloc_context3(encoder->videoCodec);
    if(!encoder->videoCodecCtx)
    {
        av_log(NULL, AV_LOG_ERROR, "No memory!\n");
        return -1;
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
        //encoder->videoCodecCtx->max_b_frames = 0;
    }

    //bind codec and codec context
    ret = avcodec_open2(encoder->videoCodecCtx, encoder->videoCodec, NULL);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Couldn't open the codec: %s\n", av_err2str(ret));
        return -1;
    }
   
    //create AVFrame
    encoder->frame = av_frame_alloc();
    if(!encoder->frame)
    {
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        return -1;
    }

    encoder->frame->width = encoder->videoCodecCtx->width;
    encoder->frame->height = encoder->videoCodecCtx->height;
    encoder->frame->format = encoder->videoCodecCtx->pix_fmt;

    ret = av_frame_get_buffer(encoder->frame, 0);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Couldn't allocate the video frame\n");
        return -1;
    }

    //create AVPacket
    encoder->pkt = av_packet_alloc();
    if(!encoder->pkt)
    {
        av_log(NULL, AV_LOG_ERROR, "NO Memory!\n");
        return -1;
    }

    encoder->videoStream = avformat_new_stream(encoder->fmtCtx, NULL);
    if (!encoder->videoStream) 
    {
        av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
        return -1;
    }
    encoder->videoStream->r_frame_rate = (AVRational){60, 1}; // For setting real frame rate
    encoder->videoStream->avg_frame_rate = (AVRational){60, 1}; // For setting average frame rate
    //the input file's time_base is wrong
    encoder->videoStream->time_base = encoder->videoCodecCtx->time_base;

    ret = avcodec_parameters_from_context(encoder->videoStream->codecpar, encoder->videoCodecCtx);
    if (ret < 0) 
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #\n");
        return -1;
    }

    // oFmtCtx->oformat = av_guess_format(NULL, dst, NULL);
    // if(!oFmtCtx->oformat)
    // {
    //     av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
    // }

    


    return 0;
}

static int prepare_Encoder_Audio(StreamContext *decoder, StreamContext *encoder)
{
    //TODO
    return 0;
}

static int prepare_Copy(AVFormatContext *avCtx, AVStream **stream, AVCodecParameters *codecParam)
{
    *stream = avformat_new_stream(avCtx, NULL);
    avcodec_parameters_copy((*stream)->codecpar, codecParam);
    return 0;
}

static int remux(AVPacket *pkt, AVFormatContext *avCtx, AVStream *inStream, AVStream *outStream)
{
    av_packet_rescale_ts(pkt, inStream->time_base, outStream->time_base);
    if(av_interleaved_write_frame(avCtx, pkt) < 0) 
    {
        av_log(NULL, AV_LOG_ERROR, "write frame error!\n");
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    //deal with arguments

    StreamContext *decoder = malloc(sizeof(StreamContext));
    StreamContext *encoder = malloc(sizeof(StreamContext));

    int copyAudio = 1;
    int copyVideo = 1;

    av_log_set_level(AV_LOG_DEBUG);
    if(argc < 3)
    {
        av_log(NULL, AV_LOG_ERROR, "the arguments must be more than 3!\n");
    }

    decoder->filename = argv[1];
    encoder->filename = argv[2];
    
    open_Media(decoder, encoder);

    ret = prepare_Decoder(decoder);
    if(ret < 0)
    {
        goto end;
    }
    
    if(!copyVideo)
    {
        ret = prepare_Encoder_Video(decoder, encoder);
        if(ret < 0)
        {
            goto end;
        }
    }else
    {
        prepare_Copy(encoder->fmtCtx, &encoder->videoStream, decoder->videoStream->codecpar);
    }

    if(!copyAudio)
    {
        ret = prepare_Encoder_Audio(decoder, encoder);
        if(ret < 0)
        {
            goto end;
        }
    }else
    {
        prepare_Copy(encoder->fmtCtx, &encoder->audioStream, decoder->audioStream->codecpar);
    }

    //binding
    ret = avio_open2(&encoder->fmtCtx->pb, encoder->filename, AVIO_FLAG_WRITE, NULL, NULL);
    if(ret < 0)
    {
        av_log(encoder->fmtCtx, AV_LOG_ERROR, "%s", av_err2str(ret));
        return -1;
    }
    /* Write the stream header, if any. */
    ret = avformat_write_header(encoder->fmtCtx, NULL);
    if (ret < 0) 
    {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        goto end;
    }

    //read video data from multimedia files to write into destination file
    while(av_read_frame(decoder->fmtCtx, decoder->pkt) >= 0)
    {
        if(decoder->pkt->stream_index == decoder->videoIdx )
        {
            if(!copyVideo)
            {
                transcode_Video(decoder, encoder);
            }else
            {
                remux(decoder->pkt, encoder->fmtCtx, decoder->videoStream, encoder->videoStream);
            }
            
            //encode(oFmtCtx, outCodecCtx, outFrame, outPkt, inStream, outStream);
        }else if(decoder->pkt->stream_index == decoder->audioIdx)
        {
            if(!copyAudio)
            {
                
            }else
            {
                remux(decoder->pkt, encoder->fmtCtx, decoder->audioStream, encoder->audioStream);
            }
        }
    }
    if(!copyVideo)
    {
        encoder->frame = NULL;
        //write the buffered frame
        encode_Video(decoder->videoStream, encoder);

    }
    
    av_write_trailer(encoder->fmtCtx);

    //free memory
end:
    if(decoder->fmtCtx)
    {
        avformat_close_input(&decoder->fmtCtx);
        decoder->fmtCtx = NULL;
    }
    if(decoder->videoCodecCtx)
    {
        avcodec_free_context(&decoder->videoCodecCtx);
        decoder->videoCodecCtx = NULL;
    }
    if (decoder->frame)
    {
        av_frame_free(&decoder->frame);
        decoder->frame = NULL;
    }
    if (decoder->pkt)
    {
        av_packet_free(&decoder->pkt);
        decoder->pkt = NULL;
    }

    if(encoder->fmtCtx && !(encoder->fmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        avio_closep(&encoder->fmtCtx->pb);
    }
    if(encoder->fmtCtx)
    {
        avformat_free_context(encoder->fmtCtx);
        encoder->fmtCtx = NULL;
    }
    if(encoder->videoCodecCtx)
    {
        avcodec_free_context(&encoder->videoCodecCtx);
        encoder->videoCodecCtx = NULL;
    }
    if(encoder->frame)
    {
        av_frame_free(&encoder->frame);
        encoder->frame = NULL;
    }
    if(encoder->pkt)
    {
        av_packet_free(&encoder->pkt);
        encoder->pkt = NULL;
    }
    return 0;
}