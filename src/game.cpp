#include "game.h"

void Game::SetupEnvironment() {
  int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  if (result < 0) {
    throw GameError(SDL_GetError());
  }
}

void Game::Init() {
  constexpr int kWindowWidth = 800;
  constexpr int kWindowHeight = 680;
  constexpr const char *kWindowTitle =
      "Hakusyu - Developed by Shinonome Yuugata";

  window_ = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, kWindowWidth,
                             kWindowHeight, SDL_WINDOW_SHOWN);
  if (window_ == nullptr) {
    throw GameError(SDL_GetError());
  }

  renderer_ = SDL_CreateRenderer(
      window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer_ == nullptr) {
    throw GameError(SDL_GetError());
  }
}

void Game::Main() {
  bool exit = false;
  SDL_Event e;

  while (!exit) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        exit = true;
      }
    }

    Draw();
  }
}

void Game::Draw() {
  SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer_);

  SDL_RenderPresent(renderer_);
}
