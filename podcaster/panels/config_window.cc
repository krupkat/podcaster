#include "podcaster/panels/config_window.h"

#include <imgui.h>

namespace podcaster {

Action ConfigWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  if (not config_) {
    ImGui::Text(
        "Couldn't connect to podcaster service, please try to restart the "
        "app.");
    return action;
  }

  if (config_->config().feed_size() == 0) {
    ImGui::Text("No feeds configured, add some in the config file:\n - %s",
                config_->config_path().c_str());
    return action;
  }

  //   auto label = version::AppNameWithVersion();
  //   ImGui::SeparatorText(label.c_str());

  //   ImGui::TextWrapped(
  //       "Tiny Podcaster is a podcast client for embedded Linux devices.\n - "
  //       "config file: %s\n - data storage: %s\n - debug logs: %s\n -
  //       repository: " "https://github.com/krupkat/podcaster",
  //       "", "", exe_path_.c_str());

  //   ImGui::SeparatorText("Changelog");
  //   ImGui::TextWrapped("%s", changelog_.c_str());
  //   ImGui::Spacing();

  return action;
}

void ConfigWindow::OpenImpl(const std::optional<ConfigDetails>& config) {
  config_ = config;
}

}  // namespace podcaster