#include "panels/about_window.h"

#include <imgui.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <spdlog/spdlog.h>

#include "dependencies.h"

namespace podcaster {

Action AboutWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  ImGui::TextUnformatted("Podcaster 0.0.1,");
  ImGui::SameLine();
  ImGui::TextLink("GPL v3");
  ImGui::Separator();

  SDL_version linked_sdl;
  SDL_GetVersion(&linked_sdl);
  const SDL_version* linked_mixer = Mix_Linked_Version();

  for (const auto& dep : kDependencies) {
    if (dep.name == "sdl") {
      ImGui::Text("%s [system] %d.%d.%d,", dep.name.c_str(), linked_sdl.major,
                  linked_sdl.minor, linked_sdl.patch);
    } else if (dep.name == "sdl_mixer") {
      ImGui::Text("%s [system] %d.%d.%d,", dep.name.c_str(),
                  linked_mixer->major, linked_mixer->minor,
                  linked_mixer->patch);
    } else {
      ImGui::Text("%s %s,", dep.name.c_str(), dep.version.c_str());
    }
    ImGui::SameLine();
    ImGui::PushID(dep.name.c_str());
    ImGui::TextLink(dep.license.c_str());
    ImGui::PopID();
  }

  ImGui::Spacing();
  return action;
}
}  // namespace podcaster