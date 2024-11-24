/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about pcm player through SDL2 API
 * 
 * SDL2 version 2.30.0
 * Tested on MacOS 14.1.2, compiled with clang 14.0.3
 */
#include <SDL.h>

#define BLOCK_SIZE 4096000

static size_t buffer_len = 0;
static Uint8 *audio_buf = NULL;
static Uint8 *audio_pos = NULL;

void read_audio_data(void *udata, Uint8 *stream, int len)
{
    if(buffer_len == 0){
        return;
    }

    SDL_memset(stream, 0 , len);

    len = (len < buffer_len)? len:buffer_len;

    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);

    audio_pos += len;
    buffer_len -= len;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    char *path = argv[1];

    FILE *audio_fd = NULL;

    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)){
        SDL_Log("Failed to initial!\n");
        return ret;
    }

    audio_fd = fopen(path, "r");
    if(!audio_fd){
        SDL_Log("Failed to open pcm file!\n");
        goto end;
    }

    audio_buf =(Uint8*) malloc(BLOCK_SIZE);
    if(!audio_buf){
        SDL_Log("Failed to alloc memory!\n");
        goto end;
    }

    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.channels = 2;
    spec.format = AUDIO_S16SYS;
    spec.silence = 0;
    spec.samples = 1024; 
    spec.callback = read_audio_data;
    spec.userdata = NULL;
    
    if(SDL_OpenAudio(&spec, NULL)){
        SDL_Log("Failed to open audio device!\n");
        goto end;
    }

    SDL_PauseAudio(0);

    do{
        buffer_len = fread(audio_buf, 1, BLOCK_SIZE, audio_fd);
        audio_pos = audio_buf;
        while (audio_pos < (audio_buf + buffer_len))
        {
           SDL_Delay(1);
        }


        if(buffer_len < 0){
            SDL_Log("Failed to read pcm data!\n");
            goto end;
        }
        
        
    }while(buffer_len != 0);

    SDL_CloseAudio();

    ret = 0;

end:
    if(audio_buf){
        free(audio_buf);
    }

    if(audio_fd){
        fclose(audio_fd);
    }

    SDL_Quit();
    
    return 0;
}