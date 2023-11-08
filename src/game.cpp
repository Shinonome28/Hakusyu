#include "game.h"

#include <SDL2/SDL_image.h>

#include <random>

#define _DEBUG_GAME

static std::random_device rd;
static std::mt19937 rng;
constexpr int division = 4;
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 680;
constexpr int kCharacterPosition = kWindowWidth / 10;

constexpr int kCollisionX = 1;
constexpr int kCollisionY = 2;
constexpr int kCollisionBoth = kCollisionX | kCollisionY;

int CheckBoxCollision(const SDL_Rect &a, const SDL_Rect &b) {
  // 分离轴算法
  const int left_a = a.x;
  const int right_a = a.x + a.w;
  const int top_a = a.y;
  const int bottom_a = a.y + a.h;
  const int left_b = b.x;
  const int right_b = b.x + b.w;
  const int top_b = b.y;
  const int bottom_b = b.y + b.h;

  int result = kCollisionBoth;
  if (right_a <= left_b || right_b <= left_a) {
    result &= ~kCollisionX;
  }
  if (bottom_a <= top_b || bottom_b <= top_a) {
    result &= ~kCollisionY;
  }

  return result;
}

void Game::SetupEnvironment() {
  int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  if (result < 0) {
    throw GameError(SDL_GetError());
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

  result = IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG;
  if (!result) {
    throw GameError(IMG_GetError());
  }

  rng.seed(rd());
}

void Game::Init() {
  constexpr const char *kWindowTitle =
      "Hakusyu - Developed by Shinonome Yuugata";

  window_ = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, kWindowWidth,
                             kWindowHeight, SDL_WINDOW_SHOWN);
  if (window_ == nullptr) {
    throw GameError(SDL_GetError());
  }
  window_width_ = kWindowWidth;
  window_height_ = kWindowHeight;

  renderer_ = SDL_CreateRenderer(
      window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer_ == nullptr) {
    throw GameError(SDL_GetError());
  }

  SDL_Surface *character_sprite = IMG_Load("images/foo.png");
  if (character_sprite == nullptr) {
    throw GameError(IMG_GetError());
  }
  SDL_SetColorKey(character_sprite, SDL_TRUE,
                  SDL_MapRGB(character_sprite->format, 0, 0xFF, 0xFF));
  character_texture_ =
      SDL_CreateTextureFromSurface(renderer_, character_sprite);
  if (character_texture_ == nullptr) {
    throw GameError(SDL_GetError());
  }
  SDL_Rect character_box;
  character_box.w = character_sprite->w;
  character_box.h = character_sprite->h;
  SDL_FreeSurface(character_sprite);
  SDL_SetTextureBlendMode(character_texture_, SDL_BLENDMODE_BLEND);

  GenNewBlock();
  blocks_.front().x = kCharacterPosition;
  for (int i = 1; i < division + 2; i++) {
    GenNewBlock();
  }

  character_box.x = kCharacterPosition;
  character_box.y = blocks_.front().y - character_box.h;
  physics_object_.Init(character_box);
}

void Game::Main() {
  bool exit = false;
  SDL_Event e;

  physics_object_.ApplyVelocity(1000, 0);
  physics_object_.ApplyForce(0, 500);
  physics_object_.SetFriction(0.0005f, 0.01f);

  while (!exit) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        exit = true;
      }
#ifdef _DEBUG_GAME
      if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
        switch (e.key.keysym.sym) {
          case SDLK_KP_0:
            ShiftBlocks(3);
            break;
          case SDLK_KP_1:
            physics_object_.ApplyVelocity(0.0, -1000.0);
            break;
          case SDLK_KP_2:
            physics_object_.ApplyVelocity(-500.0, 0.0);
            break;
        }
      }
#endif
    }

    physics_object_.Update(blocks_);
    ShiftBlocks(physics_object_.GetDeltaX());

    Draw();
  }
}

void Game::Draw() {
  SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer_);

  SDL_Rect character_box = physics_object_.GetBox();
  const SDL_Rect &first_block = blocks_.front();
  SDL_Rect character_render_rect = character_box;
  character_render_rect.x = kCharacterPosition;
  SDL_RenderCopy(renderer_, character_texture_, nullptr,
                 &character_render_rect);

  SDL_SetRenderDrawColor(renderer_, 0x0, 0x0, 0x0, 0xFF);
  for (const SDL_Rect &block : blocks_) {
    SDL_RenderFillRect(renderer_, &block);
  }

  SDL_RenderPresent(renderer_);
}

void Game::ShiftBlocks(int pixels) {
  SDL_Rect *front = &blocks_.front();
  if (front->x + front->w <= 0) {
    GenNewBlock();
    blocks_.pop_front();
    front = &blocks_.front();
  }
  for (SDL_Rect &block : blocks_) {
    block.x -= pixels;
  }
}

void Game::GenNewBlock() {
  const int min_height = static_cast<int>(0.4 * window_height_);
  const int max_height = static_cast<int>(0.8 * window_height_);
  const int block_width = static_cast<int>(window_width_ / division);
  const int min_width = static_cast<int>(0.5 * block_width);
  const int max_width = static_cast<int>(0.8 * block_width);

  int offset = 0;
  if (!blocks_.empty()) {
    offset = blocks_.back().x + blocks_.back().w;
  }
  std::uniform_int_distribution<int> width_gen(min_width, max_width);
  std::uniform_int_distribution<int> height_gen(min_height, max_height);
  int width = width_gen(rng), height = height_gen(rng);
  SDL_Rect block;
  block.w = width;
  block.h = height;
  block.x = block_width - width + offset;
  block.y = window_height_ - block.h;
  blocks_.push_back(block);
}

void PhysicsObject::Init(const SDL_Rect &box) {
  timer_ = SDL_GetTicks();
  box_ = box;
}

void PhysicsObject::ApplyForce(float f_x, float f_y) {
  f_x_ += f_x;
  f_y_ += f_y;
}

void PhysicsObject::ApplyVelocity(float v_x, float v_y) {
  v_x_ += v_x;
  v_y_ += v_y;
}

void PhysicsObject::SetFriction(float friction_x, float friction_y) {
  friction_x_ = friction_x;
  friction_y_ = friction_y;
}

bool PhysicsObject::Update(const std::deque<SDL_Rect> &blocks) {
  Uint32 current = SDL_GetTicks();
  float dt = (current - timer_) / 1000.0f;
  timer_ = current;

  float new_v_x = v_x_ + (f_x_ - friction_x_ * v_x_ * v_x_) * dt;
  float new_v_y = v_y_ + (f_y_ - friction_y_ * v_y_ * v_y_) * dt;

  int new_x = static_cast<int>(box_.x + (new_v_x + v_x_) * 0.5 * dt);
  int new_y = static_cast<int>(box_.y + (new_v_y + v_y_) * 0.5 * dt);

  if (new_y >= kWindowHeight) {
    return false;
  }

  {
    SDL_Rect new_box = {new_x, box_.y, box_.w, box_.h};
    for (const SDL_Rect &block : blocks) {
      int result = CheckBoxCollision(new_box, block);
      if (result == kCollisionBoth) {
        new_x = box_.x;
        if (result & kCollisionX) {
          new_v_x = 0;
        }
        break;
      }
    }
  }

  {
    SDL_Rect new_box = {box_.x, new_y, box_.w, box_.h};
    for (const SDL_Rect &block : blocks) {
      int result = CheckBoxCollision(new_box, block);
      if (result == kCollisionBoth) {
        new_y = box_.y;
        if (result & kCollisionY) {
          new_v_y = 0;
        }
        break;
      }
    }
  }

  delta_x = new_x - box_.x;
  delta_y = new_y - box_.y;
  // box_.x = new_x;
  box_.y = new_y;
  v_x_ = new_v_x;
  v_y_ = new_v_y;

  return false;
}

int PhysicsObject::GetDeltaX() { return delta_x; }

int PhysicsObject::GetDeltaY() { return delta_y; }

SDL_Rect PhysicsObject::GetBox() { return box_; }
