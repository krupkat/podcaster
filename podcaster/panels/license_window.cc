// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "podcaster/panels/license_window.h"

#include <algorithm>

#include <imgui.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <spdlog/spdlog.h>

#include "podcaster/dependencies.h"
#include "podcaster/panels/show_license_window.h"
#include "podcaster/version.h"

namespace podcaster {

LicenseWindow::LicenseWindow(const std::filesystem::path& exe_path)
    : exe_path_(exe_path) {
  std::transform(kDependencies.begin(), kDependencies.end(),
                 std::back_inserter(dependencies_), [&](const Dependency& dep) {
                   auto dep_copy = dep;
                   dep_copy.license_file = exe_path / ".." / "share" /
                                           "podcaster" / dep.license_file;
                   return dep_copy;
                 });
}

Action LicenseWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  auto label = version::AppNameWithVersion();
  ImGui::TextUnformatted(label.c_str());
  ImGui::SameLine();
  if (ImGui::TextLink("GPL v3")) {
    OpenSubwindow<ShowLicenseWindow>(Dependency{
        "Tiny Podcaster", version::ToString(), "GPL v3",
        exe_path_ / ".." / "share" / "podcaster" / "licenses/gpl.txt"});
  }
  ImGui::Spacing();
  ImGui::Separator();

  ImGui::Text("Noto fonts");
  ImGui::SameLine();
  if (ImGui::TextLink("OFL")) {
    OpenSubwindow<ShowLicenseWindow>(Dependency{
        "Noto fonts", "", "Open Font License",
        exe_path_ / ".." / "share" / "podcaster" / "licenses/OFL.txt"});
  }
  ImGui::Spacing();
  ImGui::Separator();

  SDL_version linked_sdl;
  SDL_GetVersion(&linked_sdl);
  const SDL_version* linked_mixer = Mix_Linked_Version();

  for (const auto& dep : dependencies_) {
#ifdef PODCASTER_HANDHELD_BUILD
    if (dep.name == "sdl") {
      ImGui::Text("%s [system] %d.%d.%d,", dep.name.c_str(), linked_sdl.major,
                  linked_sdl.minor, linked_sdl.patch);
    } else if (dep.name == "sdl_mixer") {
      ImGui::Text("%s [system] %d.%d.%d,", dep.name.c_str(),
                  linked_mixer->major, linked_mixer->minor,
                  linked_mixer->patch);
    } else {
#endif
      ImGui::Text("%s %s", dep.name.c_str(), dep.version.c_str());
#ifdef PODCASTER_HANDHELD_BUILD
    }
#endif
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