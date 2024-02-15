/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about moving or deleting a file through ffmpeg API
 * 
 * FFmpeg version 4.1.7 
 * Tested on Ubuntu 22.04, compiled with GCC 11.4.0
 */
#include <libavformat/avformat.h>

int main(int argc, char *argv[])
{
    int ret;
    //rename a file
    ret = avpriv_io_move("111.txt","222.txt");
    if(ret<0){
        av_log(NULL, AV_LOG_ERROR, "Failed to rename the file \n");
        return -1;
    }
    av_log(NULL, AV_LOG_INFO, "Success to rename\n");

    //delete a file through url
    ret = avpriv_io_delete("./mytestfile.txt");
    if(ret<0){
        av_log(NULL, AV_LOG_ERROR, "Failed to delete the file %s\n","mytestfile.txt");
        return -1;
    }
    av_log(NULL, AV_LOG_INFO, "Success to delete mytestfile.txt\n");
    return 0;
}