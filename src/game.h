#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

class GameError : public std::runtime_error {
 public:
  GameError(const char *message) : std::runtime_error(message) {}
};

class PhysicsObject {
 public:
  void Init(const SDL_Rect &box);
  void ApplyForce(float f_x, float f_y);
  void ApplyVelocity(float v_x, float v_y);
  void SetFriction(float friction_x, float friction_y);
  bool Update(const std::deque<SDL_Rect> &blocks);
  int GetDeltaX();
  int GetDeltaY();
  SDL_Rect GetBox();

 private:
  Uint32 timer_;
  float f_x_ = 0, f_y_ = 0;
  float v_x_ = 0, v_y_ = 0;
  int delta_x = 0, delta_y = 0;
  float friction_x_ = 0, friction_y_ = 0;
  SDL_Rect box_;
};

enum class GameState {
  kHelp,
  kSelectDevice,
  kGaming,
  kGameEnd,
};

class Game {
 public:
  static void SetupEnvironment();

  void Init();

  void Main();

 private:
  void RenderTexts(const std::vector<std::string> &texts, bool is_centering,
                   int margin);
  void GamingDraw();
  void ShiftBlocks(int pixels);
  void GenNewBlock();
  void StartNewGame();
  std::tuple<SDL_Texture *, SDL_Rect> GetTextTexture(const char *text);

  GameState state_ = GameState::kHelp;
  int window_width_;
  int window_height_;
  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  SDL_Texture *character_texture_ = nullptr;
  SDL_Rect character_texture_wh_;
  TTF_Font *font_;
  std::deque<SDL_Rect> blocks_;
  PhysicsObject physics_object_;
  int help_page_count_ = 0;
  bool need_rerender = true;
};
