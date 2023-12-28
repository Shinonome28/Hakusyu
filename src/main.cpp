
#include <Windows.h>

#include "game.h"

void ErrorMessageBox(const char *msg);

int main(int argc, char **argv) {
  try {
    Game::SetupEnvironment();
    Game game;
    game.Init();
    game.Main();
    game.Exit();
  } catch (GameError &e) {
    ErrorMessageBox(e.what());
  } catch (std::exception &e) {
    ErrorMessageBox(e.what());
  }
  return 0;
}

void ErrorMessageBox(const char *msg) {
  MessageBox(nullptr, msg, "Error! Please contact developer",
             MB_OK | MB_ICONERROR);
}
