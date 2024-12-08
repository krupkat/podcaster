#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <SDL.h>
#include <SDL_gamecontroller.h>
#include <spdlog/spdlog.h>

#include "imgui_utils.h"
#include "podcaster_gui.h"
#include "sdl_utils.h"

const std::string kControllerDatabasePath = "/usr/lib/gamecontrollerdb.txt";
const std::vector<std::string> kTargetController = {"ANBERNIC-keys",
                                                    "Deeplay-keys"};

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

#ifdef PODCASTER_HANDHELD_BUILD
  auto controller_ctx = sdl::FindController(kTargetController);
  spdlog::info("Controller found: {}", controller_ctx.name);
#endif

  // imgui
  auto imgui_ctx =
      imgui::Init(exe_path, window_ctx.handle.get(), window_ctx.renderer.get());
  spdlog::info("ImGui initialized: {}", imgui_ctx.imgui_ini_file->string());

  // gui
  podcaster::PodcasterClient podcaster_client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  podcaster::PodcasterGui podcaster_gui{std::move(podcaster_client)};

  // event loop
  SDL_Event event;
  bool quit = false;

  while (!quit) {
    while (SDL_PollEvent(&event) > 0) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      switch (event.type) {
        case SDL_QUIT:
          quit = true;
          break;
        case SDL_CONTROLLERBUTTONDOWN: {
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_GUIDE) {
            quit = true;
          }
        }
      }
    }

    SDL_SetRenderDrawColor(window_ctx.renderer.get(), 0, 0, 0, 255);
    SDL_RenderClear(window_ctx.renderer.get());

    imgui::Render(
        [&]() {
          podcaster_gui.Run();
          // ImGui::ShowDemoWindow();
        },
        window_ctx.renderer.get());

    SDL_RenderPresent(window_ctx.renderer.get());
  }

  return 0;
}