#include "podcaster_gui.h"

#include <imgui.h>

namespace podcaster {

Action DrawEpisode(const Episode& episode, bool queue = false) {
  Action action = {};
  ImGui::PushID(episode.episode_uri().c_str());
  ImGuiTreeNodeFlags extra_flags = queue ? ImGuiTreeNodeFlags_DefaultOpen : 0;

  if (ImGui::TreeNodeEx(episode.title().c_str(),
                        ImGuiTreeNodeFlags_NoTreePushOnOpen | extra_flags)) {
    if (ImGui::BeginChild(episode.episode_uri().c_str(), ImVec2(0, 0),
                          ImGuiChildFlags_AutoResizeY |
                              ImGuiChildFlags_NavFlattened |
                              ImGuiChildFlags_FrameStyle)) {
      ImGui::TextWrapped("%s", episode.description().c_str());

      if (queue) {
        if (ImGui::Button("Play")) {
          action |= {.type = ActionType::kPlayEpisode,
                     .extra = EpisodeExtra{episode.episode_uri()}};
        }
      } else {
        if (ImGui::Button("Download")) {
          action |= {.type = ActionType::kDownloadEpisode,
                     .extra = EpisodeExtra{episode.episode_uri()}};
        }
      }
    }
    ImGui::EndChild();
  }

  ImGui::PopID();
  return action;
}

Action PodcasterGui::Draw() {
  Action action = {};

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  const ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

  ImGui::Begin("MainWindow", nullptr, window_flags);

  ImGui::SeparatorText("Podcaster 0.0.1");

  if (ImGui::Button("Refresh")) {
    action |= {ActionType::kRefresh};
  }

  ImGui::Spacing();
  ImGui::SeparatorText("Downloaded");

  for (const auto& episode : state_.queue()) {
    action |= DrawEpisode(episode, true);
  }

  ImGui::Spacing();
  ImGui::SeparatorText("Available to download");

  for (const auto& podcast : state_.podcasts()) {
    ImGui::PushID(podcast.podcast_uri().c_str());
    if (ImGui::TreeNodeEx(
            podcast.title().c_str(),
            ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Framed)) {
      for (const auto& episode : podcast.episodes()) {
        ImGui::PushID(episode.episode_uri().c_str());
        action |= DrawEpisode(episode);
        ImGui::PopID();
      }
    }
    ImGui::PopID();
  }

  ImGui::End();

  return action;
}

void PodcasterGui::Run() {
  auto action = Draw();
  switch (action.type) {
    case ActionType::kRefresh:
      client_.Refresh();
      state_ = client_.GetState();
      break;
    case ActionType::kDownloadEpisode:
      spdlog::info("Download episode: {}",
                   std::get<EpisodeExtra>(action.extra).episode_uri);
      break;
    case ActionType::kPlayEpisode:
      spdlog::info("Play episode: {}",
                   std::get<EpisodeExtra>(action.extra).episode_uri);
      break;
    default:
      break;
  }
}

}  // namespace podcaster