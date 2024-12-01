
#include "sdl_utils.h"

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include <SDL.h>
#include <SDL_mixer.h>
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

  SDL_version compiled_mixer;
  SDL_MIXER_VERSION(&compiled_mixer);
  const SDL_version* linked_mixer = Mix_Linked_Version();

  spdlog::info("Compiled against SDL_mixer version {}.{}.{}",
               compiled_mixer.major, compiled_mixer.minor,
               compiled_mixer.patch);
  spdlog::info("Linking against SDL_mixer version {}.{}.{}",
               linked_mixer->major, linked_mixer->minor, linked_mixer->patch);
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

SDLGameControllerContext FindController(
    std::vector<std::string> target_controller) {
  int joysticks = SDL_NumJoysticks();
  for (int i = 0; i < joysticks; i++) {
    std::string joystick_name = SDL_JoystickNameForIndex(i);
    if (SDL_IsGameController(i) == SDL_FALSE) {
      continue;
    }

    auto controller = SDLGameControllerPtr{SDL_GameControllerOpen(i),
                                           {&SDL_GameControllerClose}};
    if (controller) {
      std::string controller_name = SDL_GameControllerName(controller.get());
      if (std::find(target_controller.begin(), target_controller.end(),
                    controller_name) != target_controller.end()) {
        return {controller_name, std::move(controller)};
      }
    }
  }
  utils::Throw<std::runtime_error>("Couldn't find controller");
  return {"", {nullptr, {&SDL_GameControllerClose}}};
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