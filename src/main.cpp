
#include "game.h"

int main(int argc, char **argv) {
  try {
    Game::SetupEnvironment();
    Game game;
    game.Init();
    game.Main();
  } catch (GameError &e) {
  }
  return 0;
}
