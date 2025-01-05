// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <SDL_keycode.h>
#include <spdlog/spdlog.h>

#include "podcaster/imgui_utils.h"
#include "podcaster/podcaster_gui.h"
#include "podcaster/sdl_utils.h"

const std::string kControllerDatabasePath = "/usr/lib/gamecontrollerdb.txt";
const std::vector<std::string> kPreferredController = {
    "ANBERNIC-keys", "Deeplay-keys", "Anbernic"};

#ifdef PODCASTER_HANDHELD_BUILD
const std::string kTargetDriver = "mali";
#else
const std::string kTargetDriver = "wayland";
#endif

int main(int /*unused*/, char** /*unused*/) {
  SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE,
              kControllerDatabasePath.c_str());

  // sdl
  sdl::VerifyVersion();
  auto sdl_ctx = sdl::Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
  spdlog::info("SDL initialized");

  int driver_index = sdl::FindDriver(kTargetDriver);
  auto window_ctx = sdl::InitWindow(
      driver_index, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  spdlog::info("Window initialized: {}x{}", window_ctx.width,
               window_ctx.height);

  auto exe_path = sdl::GetExePath();
  spdlog::info("Exe path: {}", exe_path.string());

  // imgui
  auto imgui_ctx =
      imgui::Init(exe_path, window_ctx.handle.get(), window_ctx.renderer.get());
  spdlog::info("ImGui initialized: {}", imgui_ctx.imgui_ini_file->string());

#ifdef PODCASTER_HANDHELD_BUILD
  auto controller_ctx = sdl::FindController(kPreferredController);
  spdlog::info("Controller found: {}", controller_ctx.name);
  imgui::SetController(controller_ctx.handle.get());
#endif

  // gui
  podcaster::PodcasterClient podcaster_client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  podcaster::PodcasterGui podcaster_gui{std::move(podcaster_client), exe_path};

  // event loop
  SDL_Event event;
  bool quit = false;

  while (!quit) {
    podcaster::Action action = {podcaster::ActionType::kIdle};

    while (SDL_PollEvent(&event) > 0) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      switch (event.type) {
        case SDL_QUIT:
          quit = true;
          break;
        case SDL_CONTROLLERBUTTONDOWN: {
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK ||
              event.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
            quit = true;
          }
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER ||
              event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
            action |= podcaster::Action{podcaster::ActionType::kFlipPanes};
          }
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            action |= podcaster::Action{podcaster::ActionType::kScrollUp};
          }
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
            action |= podcaster::Action{podcaster::ActionType::kScrollDown};
          }
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
            action |= podcaster::Action{podcaster::ActionType::kBack};
          }
          break;
        }
        case SDL_KEYDOWN: {
          if (event.key.keysym.sym == SDLK_f) {
            action |= podcaster::Action{podcaster::ActionType::kFlipPanes};
          }
          if (event.key.keysym.sym == SDLK_UP) {
            action |= podcaster::Action{podcaster::ActionType::kScrollUp};
          }
          if (event.key.keysym.sym == SDLK_DOWN) {
            action |= podcaster::Action{podcaster::ActionType::kScrollDown};
          }
          if (event.key.keysym.sym == SDLK_r) {
            action |= podcaster::Action{podcaster::ActionType::kBack};
          }
          break;
        }
        default:
          break;
      }
    }

    SDL_SetRenderDrawColor(window_ctx.renderer.get(), 0, 0, 0, 255);
    SDL_RenderClear(window_ctx.renderer.get());

    imgui::Render(
        [&]() {
          podcaster_gui.Run(action);
        },
        window_ctx.renderer.get());

    SDL_RenderPresent(window_ctx.renderer.get());
  }

  return 0;
}