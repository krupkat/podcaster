#include "panels/about_window.h"

#include <imgui.h>
#include <SDL.h>
#include <spdlog/spdlog.h>


namespace podcaster {

// self.requires("imgui/1.91.5-docking")
// self.requires("sdl/2.28.5")
// self.requires("sdl_mixer/2.8.0")
// self.requires("spdlog/1.15.0")
// self.requires("curlpp/0.8.1.cci.20240530")
// self.requires("grpc/1.67.1")
// self.requires("pugixml/1.14")
// self.requires("tidy-html5/5.8.0")

Action AboutWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  ImGui::TextUnformatted("Podcaster 0.0.1");
  ImGui::Separator();
  ImGui::Text("ImGui version: %s", IMGUI_VERSION);

  SDL_version linked;
  SDL_GetVersion(&linked);

  ImGui::Text("SDL version: %d.%d.%d", linked.major, linked.minor,
              linked.patch);

  ImGui::Text("spdlog version: %d.%d.%d", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR,
              SPDLOG_VER_PATCH);

  ImGui::Spacing();
  return action;
}
}  // namespace podcaster