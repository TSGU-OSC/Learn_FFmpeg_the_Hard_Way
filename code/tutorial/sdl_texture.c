/*
 * copyright (c) 2024 Jack Lau
 * 
 * This file is a tutorial about texture rendering through SDL2 API
 * 
 * SDL2 version 2.30.0 
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

    SDL_Texture *texture;

    SDL_Rect rect;
    rect.w = 30;
    rect.h = 30;

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



    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, 640, 480);
    if(!texture){
        SDL_Log("Failed to Create texture!\n");
        goto drender;
    }
    do{
        
        SDL_PollEvent(&event);
        switch (event.type)
        {
        case SDL_QUIT:
            quit = 0;
            break;
        
        default:
            SDL_Log("event type is %d",event.type); 
        }

       
        rect.x = rand() % 600;
        rect.y = rand() % 450;

        SDL_SetRenderTarget(render, texture);
        SDL_SetRenderDrawColor(render, 0 , 0, 0, 0);
        SDL_RenderClear(render);

        SDL_RenderDrawRect(render, &rect);
        SDL_SetRenderDrawColor(render, 255, 0 , 0 , 0);
        SDL_RenderFillRect(render, &rect);

        SDL_SetRenderTarget(render, NULL);
        SDL_RenderCopy(render, texture, NULL, NULL);
        
        SDL_RenderPresent(render);

    }while (quit);
drender:  
    SDL_DestroyTexture(texture);

dwindow:
    SDL_DestroyWindow(window);

end:
    SDL_Quit();
    return 0;
}