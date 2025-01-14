#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 300

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO) || !TTF_Init()) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Couldn't initialize SDL or SDL_ttf: %s", SDL_GetError());
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow("Discrub", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY);

  unsigned int running = 1;
  SDL_Event event;
  while (running) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT:
          running = 0;
          break;
      }
    }
  }
  TTF_Quit();
  SDL_DestroyWindow(window);
  SDL_Quit();
}
