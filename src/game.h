#pragma once

#include <SDL2/SDL.h>

#include <queue>
#include <stdexcept>

class GameError : public std::runtime_error {
 public:
  GameError(const char *message) : std::runtime_error(message) {}
};

class PhysicsObject {};

class Game {
 public:
  static void SetupEnvironment();

  void Init();

  void Main();

 private:
  void Draw();
  void ShiftBlocks(int pixels);
  void GenNewBlock();

  int window_width_;
  int window_height_;
  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  SDL_Texture *character_texture_ = nullptr;
  SDL_Rect character_texture_size_;
  std::deque<SDL_Rect> blocks_;
};