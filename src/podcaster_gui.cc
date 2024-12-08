#include "podcaster_gui.h"

#include <imgui.h>

#include "database_utils.h"

namespace podcaster {

Action DrawEpisode(const Podcast& podcast, const Episode& episode) {
  Action action = {};
  ImGui::PushID(episode.episode_uri().c_str());
  ImGuiTreeNodeFlags extra_flags =
      episode.download_status() == DownloadStatus::DOWNLOAD_SUCCESS
          ? ImGuiTreeNodeFlags_DefaultOpen
          : 0;

  if (ImGui::TreeNodeEx(episode.title().c_str(),
                        ImGuiTreeNodeFlags_NoTreePushOnOpen | extra_flags)) {
    if (ImGui::BeginChild(episode.episode_uri().c_str(), ImVec2(0, 0),
                          ImGuiChildFlags_AutoResizeY |
                              ImGuiChildFlags_NavFlattened |
                              ImGuiChildFlags_FrameStyle)) {
      ImGui::TextWrapped("%s", episode.description().c_str());

      if (episode.download_status() == DownloadStatus::DOWNLOAD_SUCCESS) {
        if (ImGui::Button("Play")) {
          action |= {.type = ActionType::kPlayEpisode,
                     .extra = EpisodeExtra{podcast.podcast_uri(),
                                           episode.episode_uri()}};
        }
      }
      if (episode.download_status() == DownloadStatus::NOT_DOWNLOADED ||
          episode.download_status() == DownloadStatus::DOWNLOAD_ERROR) {
        if (ImGui::Button("Download")) {
          action |= {.type = ActionType::kDownloadEpisode,
                     .extra = EpisodeExtra{podcast.podcast_uri(),
                                           episode.episode_uri()}};
        }
      }
      if (episode.download_status() == DownloadStatus::DOWNLOAD_IN_PROGRESS) {
        ImGui::ProgressBar(episode.download_progress());
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

  for (const auto& podcast : state_.podcasts()) {
    for (auto iter = podcast.episodes().rbegin();
         iter != podcast.episodes().rend(); ++iter) {
      const auto& episode = *iter;
      if (episode.download_status() == DownloadStatus::DOWNLOAD_SUCCESS) {
        action |= DrawEpisode(podcast, episode);
      }
    }
  }

  ImGui::Spacing();
  ImGui::SeparatorText("Available to download");

  for (const auto& podcast : state_.podcasts()) {
    ImGui::PushID(podcast.podcast_uri().c_str());
    if (ImGui::TreeNodeEx(
            podcast.title().c_str(),
            ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Framed)) {
      for (auto iter = podcast.episodes().rbegin();
           iter != podcast.episodes().rend(); ++iter) {
        const auto& episode = *iter;
        ImGui::PushID(episode.episode_uri().c_str());
        action |= DrawEpisode(podcast, episode);
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
      refresh_future_ =
          std::async(std::launch::async, [&] { return client_.Refresh(); });
      break;
    case ActionType::kDownloadEpisode: {
      const auto& extra = std::get<EpisodeExtra>(action.extra);
      spdlog::info("Download episode: {}", extra.episode_uri);
      client_.Download(extra.podcast_uri, extra.episode_uri);
      break;
    }
    case ActionType::kPlayEpisode:
      spdlog::info("Play episode: {}",
                   std::get<EpisodeExtra>(action.extra).episode_uri);
      break;
    default:
      break;
  }

  static int frame_counter = 0;
  if (frame_counter++ % 60 == 0) {
    if (refresh_future_.valid() &&
        refresh_future_.wait_for(std::chrono::seconds(0)) ==
            std::future_status::ready) {
      state_ = refresh_future_.get();
    }

    auto episode_updates = client_.ReadUpdates();
    for (const auto& update : episode_updates) {
      utils::ApplyUpdate(update, &state_);
    }
  }
}

}  // namespace podcaster