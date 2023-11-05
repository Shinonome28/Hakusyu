#pragma once

#include <SDL2/SDL.h>

#include <queue>
#include <stdexcept>

class GameError : public std::runtime_error {
 public:
  GameError(const char *message) : std::runtime_error(message) {}
};

class Game {
 public:
  static void SetupEnvironment();

  void Init();

  void Main();

 private:
  void Draw();

  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  std::deque<SDL_Rect> blocks_;
};