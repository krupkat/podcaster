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

  auto make_episode_action = [&](ActionType type) {
    return Action{
        .type = type,
        .extra = EpisodeExtra{podcast.podcast_uri(), episode.episode_uri()}};
  };

  if (ImGui::TreeNodeEx(episode.title().c_str(),
                        ImGuiTreeNodeFlags_NoTreePushOnOpen | extra_flags)) {
    if (ImGui::BeginChild(episode.episode_uri().c_str(), ImVec2(0, 0),
                          ImGuiChildFlags_AutoResizeY |
                              ImGuiChildFlags_NavFlattened |
                              ImGuiChildFlags_FrameStyle)) {
      ImGui::TextWrapped("%s", episode.description().c_str());

      if (episode.download_status() == DownloadStatus::DOWNLOAD_SUCCESS) {
        if (episode.playback_status() == PlaybackStatus::NOT_PLAYING) {
          if (ImGui::Button("Play")) {
            action |= make_episode_action(ActionType::kPlayEpisode);
          }
          ImGui::SameLine();
          if (ImGui::Button("Delete")) {
            action |= make_episode_action(ActionType::kDeleteEpisode);
          }
          ImGui::SameLine();
        } else {
          if (episode.playback_status() == PlaybackStatus::PLAYING) {
            if (ImGui::Button("Pause")) {
              action |= make_episode_action(ActionType::kPauseEpisode);
            }
            ImGui::SameLine();
          }
          if (episode.playback_status() == PlaybackStatus::PAUSED) {
            if (ImGui::Button("Resume")) {
              action |= make_episode_action(ActionType::kResumeEpisode);
            }
            ImGui::SameLine();
          }
          if (ImGui::Button("Stop")) {
            action |= make_episode_action(ActionType::kStopEpisode);
          }
          ImGui::SameLine();
        }
      }
      if (episode.download_status() == DownloadStatus::NOT_DOWNLOADED ||
          episode.download_status() == DownloadStatus::DOWNLOAD_ERROR) {
        if (ImGui::Button("Download")) {
          action |= make_episode_action(ActionType::kDownloadEpisode);
        }
        ImGui::SameLine();
      }
      if (episode.download_status() == DownloadStatus::DOWNLOAD_IN_PROGRESS) {
        std::string label = fmt::format("Downloading: {:.0f}%",
                                        episode.download_progress() * 100.0f);
        ImGui::ProgressBar(episode.download_progress(), ImVec2(-1.0f, 0.f),
                           label.c_str());
        ImGui::SameLine();
      }
      if (int start = episode.playback_progress().elapsed_ms(); start > 1) {
        int end = episode.playback_progress().total_ms();
        std::string name = episode.playback_status() == PlaybackStatus::PLAYING
                               ? "Playing"
                               : "Progress";
        std::string label = fmt::format("{}: {}s/{}s", name, start, end);
        ImGui::ProgressBar(static_cast<float>(start) / end, ImVec2(-1.0f, 0.f),
                           label.c_str());
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
    case ActionType::kDownloadEpisode:
      [[fallthrough]];
    case ActionType::kPauseEpisode:
      [[fallthrough]];
    case ActionType::kResumeEpisode:
      [[fallthrough]];
    case ActionType::kStopEpisode:
      [[fallthrough]];
    case ActionType::kDeleteEpisode:
      [[fallthrough]];
    case ActionType::kPlayEpisode: {
      const auto& extra = std::get<EpisodeExtra>(action.extra);
      client_.EpisodeAction(extra.podcast_uri, extra.episode_uri, action.type);
      break;
    }
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