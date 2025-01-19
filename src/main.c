/*#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>*/
#include <openssl/ssl.h>
#include <stdio.h>

#include "discrub_client.h"
#include "openssl_helpers.h"

/*#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 500*/

int main() {
  SSL_CTX* ctx = create_ssl_ctx();
  if (ctx == NULL) {
    goto cleanup_none;
  }
  BIO* bio = create_bio(ctx, "discord.com:443");
  if (bio == NULL) {
    goto cleanup_ctx;
  }
  printf("Connected to discord.com successfully.\n");
  const char* username = "foo";
  const char* password = "bar";
  discrub_login(bio, username, password);

  /*if (!SDL_Init(SDL_INIT_VIDEO) || !TTF_Init()) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Couldn't initialize SDL or SDL_ttf: %s", SDL_GetError());
    goto cleanup_bio;
  }
  SDL_Window* window = SDL_CreateWindow("Discrub", WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_HIGH_PIXEL_DENSITY);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
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
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
  }
  TTF_Quit();
  SDL_DestroyWindow(window);
  SDL_Quit();*/
  /*cleanup_bio:*/
  BIO_free_all(bio);
cleanup_ctx:
  SSL_CTX_free(ctx);
cleanup_none:
  EVP_cleanup();
}
