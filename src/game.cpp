#include "game.h"

#include <SDL2/SDL_image.h>

#include <random>

#define _DEBUG_SCROLLING

static std::random_device rd;
static std::mt19937 rng;
constexpr int division = 4;
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 680;
constexpr int kCharacterPosition = kWindowWidth / 10;

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
  character_texture_size_.w = character_sprite->w;
  character_texture_size_.h = character_sprite->h;
  SDL_FreeSurface(character_sprite);
  SDL_SetTextureBlendMode(character_texture_, SDL_BLENDMODE_BLEND);

  for (int i = 0; i < division + 2; i++) {
    GenNewBlock();
  }
  blocks_.front().x = kCharacterPosition;
}

void Game::Main() {
  bool exit = false;
  SDL_Event e;

  while (!exit) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        exit = true;
      }
#ifdef _DEBUG_SCROLLING
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_KP_0) {
        ShiftBlocks(3);
      }
#endif
    }

    Draw();
  }
}

void Game::Draw() {
  SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer_);

  const SDL_Rect &first_block = blocks_.front();
  SDL_Rect character_render_rect;
  character_render_rect.x = kCharacterPosition;
  character_render_rect.y = first_block.y - character_texture_size_.h;
  character_render_rect.h = character_texture_size_.h;
  character_render_rect.w = character_texture_size_.w;
  SDL_RenderCopy(renderer_, character_texture_, nullptr,
                 &character_render_rect);

  SDL_SetRenderDrawColor(renderer_, 0x0, 0x0, 0x0, 200);
  int offset = 0;
  for (const SDL_Rect &block : blocks_) {
    offset += block.x;
    SDL_Rect draw_rect;
    draw_rect.x = offset;
    draw_rect.y = block.y;
    draw_rect.h = block.h;
    draw_rect.w = block.w;
    SDL_RenderFillRect(renderer_, &draw_rect);
    offset += draw_rect.w;
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
  front->x -= pixels;
}

void Game::GenNewBlock() {
  const int min_height = static_cast<int>(0.4 * window_height_);
  const int max_height = static_cast<int>(0.8 * window_height_);
  const int block_width = static_cast<int>(window_width_ / division);
  const int min_width = static_cast<int>(0.5 * block_width);
  const int max_width = static_cast<int>(0.8 * block_width);

  std::uniform_int_distribution<int> width_gen(min_width, max_width);
  std::uniform_int_distribution<int> height_gen(min_height, max_height);
  int width = width_gen(rng), height = height_gen(rng);
  SDL_Rect block;
  block.w = width;
  block.h = height;
  block.x = block_width - width;
  block.y = window_height_ - block.h;
  blocks_.push_back(block);
}
