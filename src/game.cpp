#include "game.h"

#include <SDL2/SDL_image.h>

#include <iomanip>
#include <random>
#include <sstream>

#define _DEBUG_GAME

static std::random_device rd;
static std::mt19937 rng;
static Uint8 *gCurrentRecordingBuffer = nullptr;
static size_t gCurrentRecordingBufferPosition = 0;

constexpr int division = 6;
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 680;
constexpr int kCharacterPosition = kWindowWidth / 10;
constexpr int kDefaultLineMargin = 20;
constexpr int kDefaultPointSize = 28;
constexpr int kMaxRecordTime = 1;
constexpr int kBufferRecordTime = 2;
constexpr int kSuggestClipTime = 10;

constexpr float kRelativeAmplitudeToVerticalSpeed = 50.0f;
constexpr float kRelativeAmplitudeToHorizontalSpeed = 30.0f;
constexpr float kFrictionHorizontal = 0.05f;
constexpr float kFrictionVertical = 0.001f;
constexpr float kGravity = 500.0f;
constexpr float kSimulateRelativeAmplitude = 1.0f;

constexpr SDL_Color kDefaultTextColor = {0, 0, 0, 0xFF};
constexpr SDL_Color kNotHitBlockColor = {0, 0, 0, 0xFF};
constexpr SDL_Color kHitBlockColor = {65, 105, 225, 0xFF};

constexpr int kCollisionX = 1;
constexpr int kCollisionY = 2;
constexpr int kCollisionBoth = kCollisionX | kCollisionY;

constexpr const char *kWindowTitle = "Hakusyu - Developed by Shinonome Yuugata";

const std::vector<std::vector<std::string>> kHelpTexts{
    {"Hello, welcome to Hakusyu!",
     "Hakusyu is a platform game using sound control",
     "That means, by the volume of applaud",
     "You can always press <ESC> to exit this game.",
     "<Press Enter to Continue>",
     "A advice: you'll better turn off input method"},
    {"Now please select your microphone by entering number",
     "If you see garbled characters, don't worry", "Just select a random one",
     "<Press Enter to Continue>"}};

const std::vector<std::string> kPromptNoRecorderDevice{
    "Bro you need a microphone to play this game", "<Press Any Key To Exit>"};

const std::vector<std::string> kPromptSelectRecorderDevice{
    "Enter a Number Key to Select Recorder Device",
    "You can use either num key rows or num pad",
};

const std::vector<std::string> kPromptRecordingMinimumVolume{
    "Please keep quiet. We will now record your minimum volume",
    "<Press Enter To Start>"};

const std::vector<std::string> kPromptRecordingMaximumVolume{
    "Please applaud as loudly as you can.",
    "We will record your maximum volume", "<Press Enter To Start>"};

const std::vector<std::string> kPromptReadyForGame{
    "You are all set!", "<Please Enter to Start Game>",
    "For your interest, this is your game arguments: "};

const std::vector<std::string> kPromptGameEnd{
    "Game End!",
    "Now you can press <Enter> to start a new game.",
    "Your score is, very surprisingly: ",
};

inline int CheckBoxCollision(const SDL_Rect &a, const SDL_Rect &b) {
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

template <typename T>
T clap(T &val, const T &min, const T &max) {
  if (val < min) {
    return min;
  } else if (val > max) {
    return max;
  }
  return val;
}

// 符号函数
inline float Sign(float f) { return f < 0.0f ? -1.0f : 1.0f; }

// The only proper way to do this
inline int GetNumberOfKey(SDL_Keycode key) {
  switch (key) {
    case SDLK_0:
    case SDLK_KP_0:
      return 0;

    case SDLK_1:
    case SDLK_KP_1:
      return 1;

    case SDLK_2:
    case SDLK_KP_2:
      return 0;

    case SDLK_3:
    case SDLK_KP_3:
      return 3;

    case SDLK_4:
    case SDLK_KP_4:
      return 4;

    case SDLK_5:
    case SDLK_KP_5:
      return 5;

    case SDLK_6:
    case SDLK_KP_6:
      return 6;

    case SDLK_7:
    case SDLK_KP_7:
      return 7;

    case SDLK_8:
    case SDLK_KP_8:
      return 8;

    case SDLK_9:
    case SDLK_KP_9:
      return 9;

    default:
      return -1;
  }
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

  result = TTF_Init();
  if (result != 0) {
    throw GameError(TTF_GetError());
  }

  rng.seed(rd());
}

void Game::Init() {
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
  character_texture_wh_.w = character_sprite->w;
  character_texture_wh_.h = character_sprite->h;
  SDL_FreeSurface(character_sprite);
  SDL_SetTextureBlendMode(character_texture_, SDL_BLENDMODE_BLEND);

  font_ = TTF_OpenFont("fonts/lazy.ttf", kDefaultPointSize);
  if (font_ == nullptr) {
    throw GameError(TTF_GetError());
  }
}

void Game::StartNewGame() {
  blocks_.clear();
  blocks_hit_state_.clear();
  GenNewBlock();
  blocks_hit_state_.front() = true;
  blocks_.front().x = kCharacterPosition;
  for (int i = 1; i < division * 2; i++) {
    GenNewBlock();
  }

  SDL_Rect character_box;
  character_box.w = character_texture_wh_.w;
  character_box.h = character_texture_wh_.h;
  character_box.x = kCharacterPosition;
  character_box.y = blocks_.front().y - character_box.h;
  physics_object_.Init(character_box);
  physics_object_.ApplyForce(0, kGravity);
  physics_object_.SetFriction(kFrictionHorizontal, kFrictionVertical);
  score_ = 0;
  state_ = GameState::kGaming;
}

bool Game::RenderPromptToSelectRecorderDevices() {
  auto devices = Recorder::GetRecorderDevices();
  // auto devices = std::vector<std::string>{};

  std::vector<std::string> audio_device_prompts;
  if (devices.size() > 0) {
    audio_device_prompts.insert(audio_device_prompts.end(),
                                kPromptSelectRecorderDevice.begin(),
                                kPromptSelectRecorderDevice.end());
  } else {
    audio_device_prompts.insert(audio_device_prompts.end(),
                                kPromptNoRecorderDevice.begin(),
                                kPromptNoRecorderDevice.end());
  }
  for (int i = 0; i < devices.size(); i++) {
    audio_device_prompts.push_back(std::to_string(i) + ": " + devices[i]);
  }
  RenderTexts(audio_device_prompts, true, kDefaultLineMargin);
  return devices.size() > 0;
}

float Game::GetRelativeAmplitude(float real_amplitude) {
  float r = (real_amplitude - minimum_amplitude_) /
            (maximum_amplitude_ - minimum_amplitude_);
  return clap(r, 0.0f, 1.0f);
}

void Game::Main() {
  bool exit = false;
  SDL_Event e;

  // StartNewGame();
  while (!exit) {
  main_loop_begin:
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        exit = true;
      } else if (e.type == SDL_KEYDOWN) {
        auto key = e.key.keysym.sym;
        int result;
        switch (state_) {
          case GameState::kHelp:
            if (key == SDLK_RETURN) {
              help_page_count_++;
              if (help_page_count_ >= kHelpTexts.size()) {
                help_page_count_ = 0;
                state_ = GameState::kSelectDevice;
              }
              need_rerender = true;
            }
            break;
          case GameState::KWaitingToExit:
            exit = true;
            break;
          case GameState::kSelectDevice:
            result = GetNumberOfKey(key);
            if (result != -1) {
              recorder_.ActivateRecorderDevice(result);
              state_ = GameState::kRecordingMinimumVolume;
              need_rerender = true;
            }
            break;
          case GameState::kRecordingMinimumVolume:
            if (key == SDLK_RETURN) {
              recorder_.StartRecording();
            }
            break;
          case GameState::kRecordingMaximumVolume:
            if (key == SDLK_RETURN) {
              temp_timer_ = SDL_GetTicks();
            }
            break;
          case GameState::kReadyForGame:
          case GameState::kGameEnd:
            if (key == SDLK_RETURN) {
              StartNewGame();
              recorder_.StopRecording();
              recorder_.DropRecordingResult();
              recorder_.StartRecording();
            }
            break;
        }
      }

#ifdef _DEBUG_GAME
      if (state_ == GameState::kGaming && e.type == SDL_KEYDOWN &&
          e.key.repeat == 0) {
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

    recorder_.FrameUpdate();

    switch (state_) {
      case GameState::kHelp:
        if (need_rerender) {
          RenderTexts(kHelpTexts[help_page_count_], true, kDefaultLineMargin);
          need_rerender = false;
        }
        break;
      case GameState::kSelectDevice:
        if (need_rerender) {
          if (!RenderPromptToSelectRecorderDevices()) {
            state_ = GameState::KWaitingToExit;
          }
          need_rerender = false;
        }
        break;
      case GameState::kRecordingMinimumVolume:
        if (need_rerender) {
          RenderTexts(kPromptRecordingMinimumVolume, true, kDefaultLineMargin);
          need_rerender = false;
        }
        if (recorder_.HasStopped() && recorder_.GetAverageAmplitude() > 0) {
          state_ = GameState::kRecordingMaximumVolume;
          need_rerender = true;
          minimum_amplitude_ = recorder_.GetAverageAmplitude();
          recorder_.DropRecordingResult();
        }
        break;
      case GameState::kRecordingMaximumVolume:
        if (need_rerender) {
          RenderTexts(kPromptRecordingMaximumVolume, true, kDefaultLineMargin);
          need_rerender = false;
        }
        if (temp_timer_ != -1 && (SDL_GetTicks() - temp_timer_) > 2000) {
          recorder_.StartRecording();
          temp_timer_ = -1;
        }
        if (recorder_.HasStopped() && recorder_.GetAverageAmplitude() > 0) {
          state_ = GameState::kReadyForGame;
          need_rerender = true;
          maximum_amplitude_ = recorder_.GetAverageAmplitude();
          recorder_.DropRecordingResult();
        }
        break;
      case GameState::kReadyForGame:
        if (need_rerender) {
          auto texts_to_render = kPromptReadyForGame;
          std::stringstream ss;
          ss << std::setprecision(6) << "Minimum " << minimum_amplitude_
             << " Maximum " << maximum_amplitude_ << " Slope "
             << 1 / (maximum_amplitude_ - minimum_amplitude_);
          texts_to_render.push_back(ss.str());
          RenderTexts(texts_to_render, true, kDefaultLineMargin);
          need_rerender = false;
        }
        break;
      case GameState::kGameEnd:
        if (need_rerender) {
          auto texts_to_render = kPromptGameEnd;
          texts_to_render.push_back(std::to_string(score_));
          RenderTexts(texts_to_render, true, kDefaultLineMargin);
          need_rerender = false;
        }
        break;
      case GameState::kGaming:
        if (gCurrentRecordingBufferPosition >
            recorder_.GetSuggestClipPosition()) {
          recorder_.StopRecording();
        } else {
          continue;
        }
        const float sys_amplitude = recorder_.GetAverageAmplitude();
        const float relative_amplitude = GetRelativeAmplitude(sys_amplitude);
        // float relative_amplitude = kSimulateRelativeAmplitude;
        const float vertical_speed =
            -relative_amplitude * kRelativeAmplitudeToVerticalSpeed;
        const float horizontal_speed =
            relative_amplitude * kRelativeAmplitudeToHorizontalSpeed;
        physics_object_.ApplyVelocity(horizontal_speed, vertical_speed);
        HitDetectionResult r = physics_object_.Update(blocks_);
        if (r.hit_lower_border || r.hit_upper_border) {
          state_ = GameState::kGameEnd;
          need_rerender = true;
        }
        if (r.hit_block_id != -1) {
          if (!blocks_hit_state_[r.hit_block_id]) {
            for (int i = 0; i <= r.hit_block_id - 1; i++) {
              if (!blocks_hit_state_[i]) {
                state_ = GameState::kGameEnd;
                need_rerender = true;
                goto main_loop_begin;
              }
            }
            blocks_hit_state_[r.hit_block_id] = true;
            score_++;
          }
        }
        ShiftBlocks(physics_object_.GetDeltaX());
        GamingDraw(relative_amplitude);
        if (recorder_.CanStartRecording()) {
          recorder_.StartRecording();
        }
        break;
    }
  }
}

void Game::RenderTexts(const std::vector<std::string> &texts, bool is_centering,
                       int margin, bool standalone) {
  if (standalone) {
    SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer_);
  }
  int y_offset = 0;
  for (const std::string &text : texts) {
    auto r = GetTextTexture(text.c_str());
    SDL_Texture *texture = std::get<0>(r);
    SDL_Rect target_rect = std::get<1>(r);
    if (is_centering) {
      target_rect.x = (kWindowWidth - target_rect.w) / 2;
    } else {
      target_rect.x = 0;
    }
    target_rect.y = y_offset;
    y_offset += target_rect.h + margin;

    SDL_RenderCopy(renderer_, texture, nullptr, &target_rect);
    SDL_DestroyTexture(texture);
  }
  if (standalone) {
    SDL_RenderPresent(renderer_);
  }
}

void Game::GamingDraw(float relative_amplitude) {
  SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer_);

  RenderTexts(
      {std::to_string(static_cast<int>(std::floor(relative_amplitude * 100)))},
      false, 0, false);

  SDL_Rect character_box = physics_object_.GetBox();
  const SDL_Rect &first_block = blocks_.front();
  SDL_Rect character_render_rect = character_box;
  // character_render_rect.x = kCharacterPosition;
  SDL_RenderCopy(renderer_, character_texture_, nullptr,
                 &character_render_rect);

  for (size_t i = 0; i < blocks_.size(); i++) {
    SDL_Color c = blocks_hit_state_[i] ? kHitBlockColor : kNotHitBlockColor;
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(renderer_, &blocks_[i]);
  }

  SDL_RenderPresent(renderer_);
}

void Game::ShiftBlocks(int pixels) {
  SDL_Rect *front = &blocks_.front();
  if (front->x + front->w <= 0) {
    GenNewBlock();
    blocks_.pop_front();
    blocks_hit_state_.pop_front();
    front = &blocks_.front();
  }
  for (SDL_Rect &block : blocks_) {
    block.x -= pixels;
  }
}

void Game::GenNewBlock() {
  const int min_height = static_cast<int>(0.2 * window_height_);
  const int max_height = static_cast<int>(0.6 * window_height_);
  const int block_width = static_cast<int>(window_width_ / division);
  const int min_width = static_cast<int>(0.3 * block_width);
  const int max_width = static_cast<int>(0.6 * block_width);

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
  blocks_hit_state_.push_back(false);
}

std::tuple<SDL_Texture *, SDL_Rect> Game::GetTextTexture(const char *text) {
  SDL_Surface *s = TTF_RenderText_Solid(font_, text, kDefaultTextColor);
  if (s == nullptr) {
    throw GameError(TTF_GetError());
  }
  SDL_Texture *t = SDL_CreateTextureFromSurface(renderer_, s);
  if (t == nullptr) {
    throw GameError(SDL_GetError());
  }
  SDL_Rect size = {0, 0, s->w, s->h};
  SDL_FreeSurface(s);
  return std::make_tuple(t, size);
}

void PhysicsObject::Init(const SDL_Rect &box) {
  ResetAllVariables();
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

HitDetectionResult PhysicsObject::Update(const std::deque<SDL_Rect> &blocks) {
  HitDetectionResult hit_detection_result;
  Uint32 current = SDL_GetTicks();
  float dt = (current - timer_) / 1000.0f;
  timer_ = current;

  float new_v_x = v_x_ + (f_x_ - Sign(v_x_) * friction_x_ * v_x_ * v_x_) * dt;
  float new_v_y = v_y_ + (f_y_ - Sign(v_y_) * friction_y_ * v_y_ * v_y_) * dt;

  int new_x = static_cast<int>(box_.x + (new_v_x + v_x_) * 0.5 * dt);
  int new_y = static_cast<int>(box_.y + (new_v_y + v_y_) * 0.5 * dt);

  {
    SDL_Rect new_box = {new_x, box_.y, box_.w, box_.h};
    for (size_t i = 0; i < blocks.size(); i++) {
      int result = CheckBoxCollision(new_box, blocks[i]);
      if (result == kCollisionBoth) {
        hit_detection_result.hit_block_id = i;
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
    for (size_t i = 0; i < blocks.size(); i++) {
      int result = CheckBoxCollision(new_box, blocks[i]);
      if (result == kCollisionBoth) {
        hit_detection_result.hit_block_id = i;
        new_y = box_.y;
        if (result & kCollisionY) {
          new_v_y = 0;
        }
        break;
      }
    }
  }

  delta_x_ = new_x - box_.x;
  delta_y_ = new_y - box_.y;
  // box_.x = new_x;
  box_.y = new_y;
  v_x_ = new_v_x;
  v_y_ = new_v_y;

  if (box_.y >= kWindowHeight) {
    hit_detection_result.hit_lower_border = true;
    ;
  }
  if (box_.y <= 0) {
    hit_detection_result.hit_upper_border = true;
  }

  return hit_detection_result;
}

int PhysicsObject::GetDeltaX() { return delta_x_; }

int PhysicsObject::GetDeltaY() { return delta_y_; }

SDL_Rect PhysicsObject::GetBox() { return box_; }

void PhysicsObject::ResetAllVariables() {
  f_x_ = 0, f_y_ = 0;
  v_x_ = 0, v_y_ = 0;
  delta_x_ = 0, delta_y_ = 0;
  friction_x_ = 0, friction_y_ = 0;
}

std::vector<std::string> Recorder::GetRecorderDevices() {
  std::vector<std::string> result;
  int n = SDL_GetNumAudioDevices(SDL_TRUE);
  for (int i = 0; i < n; i++) {
    result.emplace_back(SDL_GetAudioDeviceName(i, SDL_TRUE));
  }
  return result;
}

void AudioRecordingCallback_(void *userdata, Uint8 *stream, int len) {
  std::memcpy(&gCurrentRecordingBuffer[gCurrentRecordingBufferPosition], stream,
              len);
  gCurrentRecordingBufferPosition += len;
}

Recorder::~Recorder() {
  delete[] buffer_;
  gCurrentRecordingBuffer = nullptr;
  gCurrentRecordingBufferPosition = 0;
}

void Recorder::ActivateRecorderDevice(int index) {
  SDL_AudioSpec desired_audio_spec;
  SDL_zero(desired_audio_spec);
  // following is recommended arguments for most platforms
  desired_audio_spec.freq = 44100;
  desired_audio_spec.format = AUDIO_S16;
  desired_audio_spec.channels = 2;
  desired_audio_spec.samples = 4096;
  desired_audio_spec.callback = AudioRecordingCallback_;
  id_ = SDL_OpenAudioDevice(
      SDL_GetAudioDeviceName(index, SDL_TRUE), SDL_TRUE, &desired_audio_spec,
      &recording_audio_spec_,
      SDL_AUDIO_ALLOW_ANY_CHANGE & ~SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  if (id_ == 0) {
    throw GameError(SDL_GetError());
  }

  current_index_ = index;

  const int bytes_per_sample =
      recording_audio_spec_.channels *
      (SDL_AUDIO_BITSIZE(recording_audio_spec_.format) / 8);
  const int bytes_per_second = recording_audio_spec_.freq * bytes_per_sample;
  buffer_size_ = kBufferRecordTime * bytes_per_second;
  max_buffer_position_ = kMaxRecordTime * bytes_per_second;
  suggest_clip_position_ =
      static_cast<size_t>(kSuggestClipTime / 1000.0 * bytes_per_second);

  buffer_ = new Uint8[buffer_size_];

  gCurrentRecordingBuffer = buffer_;
  gCurrentRecordingBufferPosition = 0;
  state_ = RecordingStates::kNotRecorded;
}

void Recorder::StartRecording() {
  SDL_PauseAudioDevice(id_, SDL_FALSE);
  state_ = RecordingStates::kRecording;
  gCurrentRecordingBuffer = buffer_;
  gCurrentRecordingBufferPosition = 0;
}

void Recorder::StopRecording() {
  SDL_PauseAudioDevice(id_, SDL_TRUE);
  state_ = RecordingStates::kStopped;
  buffer_position_ = gCurrentRecordingBufferPosition;
}

void Recorder::FrameUpdate() {
  if (state_ == RecordingStates::kRecording) {
    SDL_LockAudioDevice(id_);
    if (gCurrentRecordingBufferPosition > max_buffer_position_) {
      StopRecording();
    }
    SDL_UnlockAudioDevice(id_);
  }
}

bool Recorder::CanStartRecording() {
  return state_ == RecordingStates::kNotRecorded ||
         state_ == RecordingStates::kStopped;
}

bool Recorder::HasStopped() { return state_ == RecordingStates::kStopped; }

float Recorder::GetAverageAmplitude() {
  int16_t *samples = reinterpret_cast<int16_t *>(buffer_);
  size_t sample_count = buffer_position_ / sizeof(int16_t);
  if (sample_count == 0) {
    return 0.0f;
  }

  float total_amplitude = 0;
  for (size_t i = 0; i < sample_count; i++) {
    total_amplitude += std::abs(samples[i]);
  }
  return total_amplitude / sample_count;
}

void Recorder::DropRecordingResult() { buffer_position_ = 0; }

size_t Recorder::GetSuggestClipPosition() { return suggest_clip_position_; }
