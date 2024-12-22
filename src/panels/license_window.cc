#include "panels/license_window.h"

#include <imgui.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <spdlog/spdlog.h>

#include "dependencies.h"
#include "panels/show_license_window.h"
#include "version.h"

namespace podcaster {

Action LicenseWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  auto version = version::Current();

  ImGui::Text("Podcaster %d.%d.%d", version.major, version.minor,
              version.patch);
  ImGui::SameLine();
  if (ImGui::TextLink("GPL v3")) {
    OpenSubwindow<ShowLicenseWindow>(Dependency{
        "Podcaster", version::ToString(), "GPL v3", "licenses/gpl.txt"});
  }
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
      ImGui::Text("%s %s", dep.name.c_str(), dep.version.c_str());
    }
    ImGui::SameLine();
    ImGui::PushID(dep.name.c_str());
    if (ImGui::TextLink(dep.license.c_str())) {
      OpenSubwindow<ShowLicenseWindow>(dep);
    }
    ImGui::PopID();
  }

  ImGui::Spacing();
  return action;
}

}  // namespace podcaster