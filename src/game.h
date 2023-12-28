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

enum class RecordingStates {
  kNotOpenDevice,
  kNotRecorded,
  kStopped,
  kRecording,
};

class Recorder {
 public:
  static std::vector<std::string> GetRecorderDevices();

  ~Recorder();

  void ActivateRecorderDevice(int index);
  void StartRecording();
  void StopRecording();
  // This function must be called every frame
  void FrameUpdate();
  bool CanStartRecording();
  bool HasStopped();
  float GetAverageAmplitude();
  void DropRecordingResult();
  size_t GetSuggestClipPosition();

 private:
  RecordingStates state_ = RecordingStates::kNotOpenDevice;
  int current_index_ = -1;
  int id_;
  SDL_AudioSpec recording_audio_spec_;
  size_t buffer_size_, max_buffer_position_, buffer_position_,
      suggest_clip_position_;
  Uint8 *buffer_ = nullptr;
};

struct HitDetectionResult {
  bool hit_lower_border = false;
  bool hit_upper_border = false;
  int hit_block_id = -1;
};

class PhysicsObject {
 public:
  void Init(const SDL_Rect &box);
  void ApplyForce(float f_x, float f_y);
  void ApplyVelocity(float v_x, float v_y);
  void SetFriction(float friction_x, float friction_y);
  HitDetectionResult Update(const std::deque<SDL_Rect> &blocks);
  int GetDeltaX();
  int GetDeltaY();
  SDL_Rect GetBox();

 private:
  void ResetAllVariables();

  Uint32 timer_;
  float f_x_, f_y_;
  float v_x_, v_y_;
  int delta_x_, delta_y_;
  float friction_x_, friction_y_;
  SDL_Rect box_;
};

enum class GameState {
  kHelp,
  kSelectDevice,
  kGaming,
  kGameEnd,
  KWaitingToExit,
  kRecordingMinimumVolume,
  kRecordingMaximumVolume,
  kReadyForGame,
};

class Game {
 public:
  static void SetupEnvironment();

  void Init();

  void Main();

  void Exit();

 private:
  void RenderTexts(const std::vector<std::string> &texts, bool is_centering,
                   int margin, bool standalone = true);
  void GamingDraw(float relative_amplitude);
  void ShiftBlocks(int pixels);
  void GenNewBlock();
  void StartNewGame();
  bool RenderPromptToSelectRecorderDevices();
  float GetRelativeAmplitude(float real_amplitude);
  std::tuple<SDL_Texture *, SDL_Rect> GetTextTexture(const char *text);

  GameState state_ = GameState::kHelp;
  int window_width_;
  int window_height_;
  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  SDL_Texture *character_texture_ = nullptr;
  SDL_Rect character_texture_wh_;
  TTF_Font *font_ = nullptr;
  std::deque<SDL_Rect> blocks_;
  std::deque<bool> blocks_hit_state_;
  PhysicsObject physics_object_;
  Recorder recorder_;
  int help_page_count_ = 0;
  bool need_rerender = true;
  Uint32 temp_timer_ = -1;
  float minimum_amplitude_, maximum_amplitude_;
  int score_;
};
