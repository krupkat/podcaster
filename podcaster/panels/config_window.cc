#include "podcaster/panels/config_window.h"

#include <imgui.h>

namespace podcaster {

Action ConfigWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  if (not config_) {
    ImGui::Text(
        "Couldn't connect to the podcaster service, please try to restart the "
        "app.");
    ImGui::Spacing();
    return action;
  }

  if (config_->config().feed_size() == 0) {
    ImGui::Text("No feeds configured, please edit the config file:\n - %s",
                config_->config_path().c_str());
    ImGui::TextUnformatted("Specify one podcast feed per line like this:");
    ImGui::BeginChild("Config example", ImVec2(0, 0),
                      ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY);
    ImGui::TextUnformatted(
        R"(feed: "http://www.2600.com/oth-broadband.xml"
feed: "https://podcast.darknetdiaries.com/"
feed: "https://feeds.transistor.fm/full-time-nix"
)");
    ImGui::EndChild();
    ImGui::TextUnformatted(
        "After editing the config file, hit the refresh button in the main "
        "window.");
    ImGui::Spacing();
    return action;
  }

  ImGui::Text("Configured feeds:");
  for (const auto& feed : config_->config().feed()) {
    ImGui::Text(" - %s", feed.c_str());
  }
  ImGui::TextUnformatted(
      "Add podcast feeds by editing the configuration file:");
  ImGui::Text(" - %s", config_->config_path().c_str());
  ImGui::Spacing();

  return action;
}

void ConfigWindow::OpenImpl(const std::optional<ConfigInfo>& config) {
  config_ = config;
}

}  // namespace podcaster