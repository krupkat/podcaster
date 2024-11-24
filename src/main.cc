#include <string>
#include <vector>

#include <capnp/rpc-twoparty.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <kj/async-io.h>
#include <SDL.h>
#include <SDL_gamecontroller.h>
#include <SDL_mixer.h>
#include <spdlog/spdlog.h>

#include "imgui_utils.h"
#include "schema.capnp.h"
#include "sdl_utils.h"

const std::string kControllerDatabasePath = "/usr/lib/gamecontrollerdb.txt";
const std::string kTargetDriver = "mali";
const std::vector<std::string> kTargetController = {"ANBERNIC-keys", "Deeplay-keys"};

int main(int /*unused*/, char** /*unused*/) {
  SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG_FILE, kControllerDatabasePath.c_str());

  // sdl
  sdl::VerifyVersion();
  auto sdl_ctx = sdl::Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
  spdlog::info("SDL initialized");

  int mali_index = sdl::FindDriver(kTargetDriver);
  auto window_ctx =
      sdl::InitFullscreenWindow(mali_index, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  spdlog::info("Window initialized: {}x{}", window_ctx.width, window_ctx.height);

  auto exe_path = sdl::GetExePath();
  spdlog::info("Exe path: {}", exe_path.string());

  auto controller_ctx = sdl::FindController(kTargetController);
  spdlog::info("Controller found: {}", controller_ctx.name);

  // imgui
  auto imgui_ctx = imgui::Init(exe_path, window_ctx.handle.get(), window_ctx.renderer.get());
  spdlog::info("ImGui initialized: {}", imgui_ctx.imgui_ini_file->string());

  // capnproto
  auto async_io = kj::setupAsyncIo();
  auto& wait_scope = async_io.waitScope;

  kj::Network& network = async_io.provider->getNetwork();
  kj::Own<kj::NetworkAddress> addr =
      network.parseAddress("unix-abstract:podcaster").wait(wait_scope);
  kj::Own<kj::AsyncIoStream> conn = addr->connect().wait(wait_scope);

  capnp::TwoPartyClient client(*conn);

  podcaster::PodcasterService::Client podcaster_service =
      client.bootstrap().castAs<podcaster::PodcasterService>();
  spdlog::info("Connected to podcaster service");

  // event loop
  SDL_Event event;
  bool quit = false;

  auto response = podcaster_service.refreshRequest().send().wait(wait_scope);
  auto podcasts = response.getPodcasts();

  for (auto podcast : podcasts) {
    auto title = podcast.getTitle();
    spdlog::info("Podcast title: {}", title.cStr());

    auto episodes = podcast.getEpisodes();
    for (auto episode : episodes) {
      auto title = episode.getTitle();
      auto description = episode.getDescription();
      spdlog::info("Episode title: {}, description: {}", title.cStr(), description.cStr());
    }
  }

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

    imgui::Render([]() { ImGui::ShowDemoWindow(); }, window_ctx.renderer.get());

    SDL_RenderPresent(window_ctx.renderer.get());
  }

  return 0;
}