#include "podcaster/panels/about_window.h"

#include <imgui.h>

#include "podcaster/panels/utils.h"
#include "podcaster/version.h"

namespace podcaster {

Action AboutWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  auto label = version::AppNameWithVersion();
  ImGui::SeparatorText(label.c_str());

  ImGui::TextWrapped(
      "Tiny Podcaster is a podcast client for embedded Linux devices.");

  ImGui::Text(" - config file: %s", "");
  ImGui::Text(" - data storage: %s", "");
  ImGui::Text(" - debug logs: %s", exe_path_.c_str());
  ImGui::TextUnformatted(" - repository: https://github.com/krupkat/podcaster");

  ImGui::SeparatorText("Changelog");
  ImGui::TextWrapped("%s", changelog_.c_str());
  ImGui::Spacing();

  return action;
}

void AboutWindow::OpenImpl() { changelog_ = ReadFile("changelog.md"); };

}  // namespace podcaster