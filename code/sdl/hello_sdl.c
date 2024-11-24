#include <stdio.h>
#include <SDL.h>

const int screen_width = 800;
const int screen_height = 600;
int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("YourGame",
                                           SDL_WINDOWPOS_UNDEFINED,
                                           SDL_WINDOWPOS_UNDEFINED,
                                           screen_width,screen_height,
                                           SDL_WINDOW_SHOWN);
    SDL_Surface *surface = SDL_GetWindowSurface(window);
    SDL_Surface* blackground_surface = SDL_LoadBMP("hello.bmp");
    SDL_BlitSurface(blackground_surface, NULL, surface, NULL);
    SDL_UpdateWindowSurface(window);
    SDL_Delay(3000);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}