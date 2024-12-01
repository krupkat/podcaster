#include "podcaster_gui.h"

#include <imgui.h>

namespace podcaster {

void PodcasterGui::Draw() {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  const ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

  ImGui::Begin("MainWindow", nullptr, window_flags);

  ImGui::Text("Hello, world!");

  if (ImGui::Button("Refresh")) {
    client_.Refresh();
    state_ = client_.GetState();
  }

  ImGui::Text("Podcasts:");
  for (const auto& podcast : state_.podcasts()) {
    ImGui::Text("%s", podcast.title().c_str());
    ImGui::Indent();
    for (const auto& episode : podcast.episodes()) {
      ImGui::Text("%s", episode.title().c_str());
    }
    ImGui::Unindent();
  }

  ImGui::Text("Queue:");
  for (const auto& episode : state_.queue()) {
    ImGui::Text("%s", episode.title().c_str());
  }

  ImGui::End();
}

void PodcasterGui::Run() { Draw(); }

}  // namespace podcaster