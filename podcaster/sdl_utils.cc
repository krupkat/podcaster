// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sdl_utils.h"

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include <SDL.h>
#include <SDL_video.h>
#include <spdlog/spdlog.h>

namespace sdl {

void VerifyVersion() {
  SDL_version compiled;
  SDL_version linked;

  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);

  spdlog::info("Compiled against SDL version {}.{}.{}", compiled.major,
               compiled.minor, compiled.patch);
  spdlog::info("Linking against SDL version {}.{}.{}", linked.major,
               linked.minor, linked.patch);
}

int FindDriver(std::string target_driver) {
  int num_drivers = SDL_GetNumVideoDrivers();
  int driver_index = -1;

  spdlog::info("Available drivers: {}", num_drivers);
  for (int i = 0; i < num_drivers; i++) {
    std::string driver = SDL_GetVideoDriver(i);
    if (driver == target_driver) {
      driver_index = i;
    }
    spdlog::info("Driver {}: {}", i, driver);
  }

  if (driver_index == -1) {
    utils::Throw<std::runtime_error>("No mali driver available");
  }

  return driver_index;
}

std::string SwapABXYButtons(const std::string& mapping) {
  struct ButtonMapping {
    int index;
    std::string value;
  } a_btn, b_btn, x_btn, y_btn;

  std::vector<std::string> parts;
  std::string part;
  std::istringstream stream(mapping);
  int index = 0;
  while (std::getline(stream, part, ',')) {
    parts.push_back(part);
    if (auto split = std::find(part.begin(), part.end(), ':');
        split != part.end()) {
      std::string key = part.substr(0, split - part.begin());
      std::string value = part.substr(split - part.begin() + 1);
      if (key == "a") {
        a_btn = {index, value};
      } else if (key == "b") {
        b_btn = {index, value};
      } else if (key == "x") {
        x_btn = {index, value};
      } else if (key == "y") {
        y_btn = {index, value};
      }
    }
    index++;
  }

  std::swap(a_btn.value, b_btn.value);
  std::swap(x_btn.value, y_btn.value);

  parts[a_btn.index] = "a:" + a_btn.value;
  parts[b_btn.index] = "b:" + b_btn.value;
  parts[x_btn.index] = "x:" + x_btn.value;
  parts[y_btn.index] = "y:" + y_btn.value;

  std::stringstream swapped_mapping;
  for (const auto& part : parts) {
    swapped_mapping << part << ",";
  }

  return swapped_mapping.str();
}

void UpdateMapping(int joystick_index) {
  SDLCharPtr mapping = {SDL_GameControllerMappingForDeviceIndex(joystick_index),
                        &SDL_free};
  if (not mapping) {
    utils::Throw<std::runtime_error>("Error getting controller mapping: {}",
                                     SDL_GetError());
  }

  std::string mapping_str{mapping.get()};
  std::string target = "Anbernic";

  if (mapping_str.find(target) != std::string::npos) {
    auto swapped_mapping = SwapABXYButtons(mapping_str);
    spdlog::info("Swapped ABXY buttons for Anbernic controller {}",
                 swapped_mapping);

    // update sdl button mapping
    int ret = SDL_GameControllerAddMapping(swapped_mapping.c_str());
    if (ret != 0) {
      spdlog::error("Error adding swapped controller mapping: {}",
                    SDL_GetError());
    }
  }
}

SDLGameControllerContext FindController(
    std::vector<std::string> preferred_controllers) {
  std::optional<SDLGameControllerContext> controller_ctx;

  int joysticks = SDL_NumJoysticks();
  for (int i = 0; i < joysticks; i++) {
    std::string joystick_name = SDL_JoystickNameForIndex(i);
    if (SDL_IsGameController(i) == SDL_FALSE) {
      continue;
    }

    //UpdateMapping(i); this doesn't work

    auto controller = SDLGameControllerPtr{SDL_GameControllerOpen(i),
                                           {&SDL_GameControllerClose}};
    if (controller) {
      std::string controller_name = SDL_GameControllerName(controller.get());
      bool is_preferred =
          std::find_if(preferred_controllers.begin(),
                       preferred_controllers.end(),
                       [&controller_name](const std::string& target) {
                         return controller_name.starts_with(target);
                       }) != preferred_controllers.end();

      if (is_preferred or not controller_ctx) {
        controller_ctx = {controller_name, i, std::move(controller)};
      }
    }
  }

  if (not controller_ctx) {
    utils::Throw<std::runtime_error>("Couldn't find controller");
  }

  return std::move(controller_ctx.value());
}

SDLContext Init(Uint32 flags) {
  if (SDL_Init(flags) != 0) {
    utils::Throw<std::runtime_error>("Error initializing SDL: {}",
                                     SDL_GetError());
  }
  auto sdl_quit = utils::DestructorCallback([] { SDL_Quit(); });
  return SDLContext{std::move(sdl_quit)};
}

SDLWindowContext InitWindow(int driver_index, Uint32 render_flags) {
#ifdef PODCASTER_HANDHELD_BUILD
  Uint32 window_flags = SDL_WINDOW_FULLSCREEN;
#else
  Uint32 window_flags = SDL_WINDOW_SHOWN;
#endif
  SDLWindowPtr window = {
      SDL_CreateWindow("empty", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, 640, 480, window_flags),
      {&SDL_DestroyWindow}};

  if (not window) {
    utils::Throw<std::runtime_error>("Error initializing SDL window: {}",
                                     SDL_GetError());
  }

  SDLRendererPtr renderer = {
      SDL_CreateRenderer(window.get(), driver_index, render_flags),
      {&SDL_DestroyRenderer}};

  if (not renderer) {
    utils::Throw<std::runtime_error>("Error initializing SDL renderer: {}",
                                     SDL_GetError());
  }

  int width;
  int height;
  if (SDL_GetRendererOutputSize(renderer.get(), &width, &height) != 0) {
    utils::Throw<std::runtime_error>("Error getting renderer output size: {}",
                                     SDL_GetError());
  }

  return SDLWindowContext{std::move(window), std::move(renderer), width,
                          height};
}

std::filesystem::path GetExePath() {
  auto sdl_base_path =
      std::unique_ptr<char, decltype(&SDL_free)>(SDL_GetBasePath(), &SDL_free);
  if (not sdl_base_path) {
    utils::Throw<std::runtime_error>("Error getting base path: {}",
                                     SDL_GetError());
  }
  return {sdl_base_path.get()};
}

}  // namespace sdl