/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about sdl event through SDL2 API
 * 
 * FFmpeg version 5.0.3 
 * Tested on MacOS 14.1.2, compiled with clang 14.0.3
 */
#include <SDL.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int quit = 1;
    SDL_Event event;

    SDL_Window *window = NULL;
    SDL_Renderer *render = NULL;

    SDL_Init(SDL_INIT_VIDEO);
    
    window = SDL_CreateWindow("SDL2 Window",//window name
                     200,// X
                     200,// Y
                     640,// width
                     480,// height
                     SDL_WINDOW_SHOWN );

    if(!window){
        printf("Failed to Created window!");
        goto end;
    }

    render = SDL_CreateRenderer(window,-1,0);
    if(!render){
        SDL_Log("Failed to Create Render!\n");
        goto dwindow;
    }
    //set the color for drawer
    SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
    //clear render
    SDL_RenderClear(render);
    //present the image
    SDL_RenderPresent(render);
    //create sdl event to make the window could be closed 
    do{
        
        SDL_WaitEvent(&event);
        switch (event.type)
        {
        case SDL_QUIT:
            quit = 0;
            break;
        
        default:
            SDL_Log("event type is %d",event.type); 
        }
    }while (quit);
    


dwindow:
    SDL_DestroyWindow(window);

end:
    SDL_Quit();
    return 0;
}