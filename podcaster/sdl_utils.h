// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <SDL.h>

#include "podcaster/utils.h"

namespace sdl {

using SDLWindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using SDLRendererPtr =
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using SDLGameControllerPtr =
    std::unique_ptr<SDL_GameController, decltype(&SDL_GameControllerClose)>;

struct SDLContext {
  utils::DestructorCallback sdl_quit;
};

struct SDLWindowContext {
  SDLWindowPtr handle;
  SDLRendererPtr renderer;
  int width;
  int height;
};

struct SDLGameControllerContext {
  std::string name;
  SDLGameControllerPtr handle;
};

SDLContext Init(Uint32 flags);

SDLWindowContext InitWindow(int driver_index, Uint32 render_flags);

int FindDriver(std::string target_driver);

void VerifyVersion();

SDLGameControllerContext FindController(
    std::vector<std::string> target_controller);

std::filesystem::path GetExePath();

}  // namespace sdl