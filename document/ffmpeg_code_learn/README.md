# ffmpeg开发 入门教程

### 配置开发环境

下载源代码并手动编译 
[详见](../ffmpeg_develop/README.md)

### 读取视频编码信息

在任意目录下创建好你所需的代码文件
```
touch AVCodecContext.cpp
```
编辑此代码文件
```c++
#include <stdio.h>
//inlcude these ffmpeg headers you need
extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}

int main()
{
    AVFormatContext *fmt_ctx=NULL;
    AVCodecContext *codec_ctx=NULL;
    const AVCodec *codec =NULL;
    int ret;
    //you should change to your absoulte path
    const char *filename="/Users/jacklau/Movies/ffmpeg_test/input/test.mov";
    int VideoStreamIndex=-1;

    if((ret=avformat_open_input(&fmt_ctx,filename,NULL,NULL)))
    {
        av_log(NULL,AV_LOG_ERROR,"cannot open input file\n");
        goto end;
    }

    if((ret=avformat_find_stream_info(fmt_ctx,NULL))<0)
    {
        av_log(NULL,AV_LOG_ERROR,"cannot get stream info\n");
        goto end;
    }

    for(int i=0;i < fmt_ctx->nb_streams;i++)
    {
        if(fmt_ctx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            VideoStreamIndex=i;
            break;
        }
    }

    if(VideoStreamIndex<0)
    {
        av_log(NULL,AV_LOG_ERROR,"no stream found\n");
        goto end;
    }

    av_dump_format(fmt_ctx,VideoStreamIndex,filename,false);

    codec_ctx=avcodec_alloc_context3(NULL);
    
    if((ret=avcodec_parameters_to_context(codec_ctx,fmt_ctx->streams[VideoStreamIndex]->codecpar))<0)
    {
        av_log(NULL,AV_LOG_ERROR,"cannot get codec params\n");
        goto end;
    }
    
    codec=avcodec_find_decoder(codec_ctx->codec_id);

    if(codec==NULL)
    {
        av_log(NULL,AV_LOG_ERROR,"cannot find decoder\n");
        goto end;
    }

    if((ret=avcodec_open2(codec_ctx,codec,NULL))<0)
    {
        av_log(NULL,AV_LOG_ERROR,"cannot open decoder\n");
        goto end;
    }

    fprintf(stderr,"\nDecodeding code is : %s \n",codec->name);

    avformat_close_input(&fmt_ctx);
    //end this execute    
end:
    if(codec_ctx)
        avcodec_close(codec_ctx);
    if(fmt_ctx)
        avformat_close_input(&fmt_ctx);

    return 0;
}
```

#### 编译
输入以下指令
```
g++ -I/path/to/ffmpeg/include -L/path/to/ffmpeg/lib AVCodecContext.cpp -lavformat -lavutil -lavcodec -o AVCodecContext  
```
* `-I` 指指定g++编译前加入的外部库（代码，由头文件和代码组成），后跟实际的库路径

* `-L` 指指定g++编译前加入的外部静态or动态库，后跟实际的库路径

* `-lavformat` `-lavutil` `-lavcodec` 指引用ffmpeg这三个静态or动态库

#### 运行
输入下行代码以运行编译好的程序
```
./AVCodecContext 
```
屏幕正确输出视频所用编码即为成功


