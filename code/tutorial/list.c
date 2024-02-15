/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about listing files through ffmpeg API
 * 
 * FFmpeg version 4.1.7 
 * Tested on Ubuntu 22.04, compiled with GCC 11.4.0
 */
#include <libavformat/avformat.h>

int main(int argc, char *argv[])
{
    int ret;
    //init
    AVIODirContext *ctx=NULL;
    AVIODirEntry *entry=NULL;
    av_log_set_level(AV_LOG_INFO);
    //open the directory
    ret = avio_open_dir(&ctx, "./", NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "failed to open the directory:%s\n",av_err2str(ret));
        goto end;
    }
    //read the files
    while(1){
        ret = avio_read_dir(ctx, &entry);
        if(ret < 0){
            av_log(NULL, AV_LOG_ERROR, "failed to read the directory:%s\n",av_err2str(ret));
            goto end;
        }
        //if all of the files listed, then break the loop
        if(!entry){
            break;
        }
        //print the info of file's size and name
        av_log(NULL, AV_LOG_INFO, "%12"PRId64" %s \n",
                entry->size,
                entry->name);
        //free the memory of entry
        avio_free_directory_entry(&entry);
    }
    
end:
    //free the memory of ctx
    avio_close_dir(&ctx);

    return 0;
}