#include "podcaster_gui.h"

#include <imgui.h>

#include "database_utils.h"
#include "imgui_utils.h"

namespace podcaster {

Action DrawEpisode(const Podcast& podcast, const Episode& episode,
                   bool show_podcast_title = false) {
  Action action = {};
  ImGui::PushID(episode.episode_uri().c_str());
  ImGuiTreeNodeFlags extra_flags =
      episode.download_status() == DownloadStatus::DOWNLOAD_SUCCESS ||
              episode.download_status() == DownloadStatus::DOWNLOAD_IN_PROGRESS
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
      if (episode.description_short().size() > 0) {
        ImGui::TextWrapped("%s", episode.description_short().c_str());
      }
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
      if (episode.description_long().size() > 0) {
        if (ImGui::Button("Show more")) {
          ImGui::OpenPopup("Show more");
        }
        ImGui::SameLine();
        if (ImGui::BeginPopup("Show more")) {
          ImGui::TextWrapped("%s", episode.description_long().c_str());
          ImGui::EndPopup();
        }
      }
      if (episode.download_status() == DownloadStatus::DOWNLOAD_IN_PROGRESS) {
        if (ImGui::Button("Cancel")) {
          action |= make_episode_action(ActionType::kCancelDownload);
        }
        ImGui::SameLine();
        std::string label = std::format("Downloading: {:.0f}%",
                                        episode.download_progress() * 100.0f);
        ImGui::ProgressBar(episode.download_progress(), ImVec2(-1.0f, 0.f),
                           label.c_str());
        ImGui::SameLine();
      }
      if (int start = episode.playback_progress().elapsed_ms(); start > 0) {
        int end = episode.playback_progress().total_ms();
        std::string name = episode.playback_status() == PlaybackStatus::PLAYING
                               ? "Playing"
                               : "Progress";
        std::string label =
            std::format("{}: {} s / {} s", name, start / 1000, end / 1000);
        ImGui::ProgressBar(static_cast<float>(start) / end, ImVec2(-1.0f, 0.f),
                           label.c_str());
      }
    }
    ImGui::EndChild();
  }

  ImGui::PopID();
  return action;
}

void DrawRefreshAnimation() {
  static int frame_counter = 0;
  static const char* dots[] = {"", ".", "..", "..."};
  ImGui::Text("In progress%s", dots[(frame_counter++ / 16) % 4]);
}

Action PodcasterGui::Draw(const Action& incoming_action) {
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

  bool top_row_in_focus = false;
  bool active_refresh = refresh_future_.valid();

  imgui::EnableIf(not active_refresh, [&] {
    if (ImGui::Button("Refresh")) {
      action |= {ActionType::kRefresh};
    }
  });
  if (active_refresh) {
    ImGui::SameLine();
    DrawRefreshAnimation();
  }

  top_row_in_focus |= ImGui::IsItemFocused();

  // scroll to top if first item is focused
  if (top_row_in_focus) {
    if (not last_top_row_in_focus_) {
      ImGui::SetScrollY(0.0f);
    }
  }
  last_top_row_in_focus_ = top_row_in_focus;

  ImGui::SeparatorText("Episodes");

  ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
  if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
    ImGuiTabItemFlags flags =
        selected_tab_ == 1 and incoming_action.type == ActionType::kFlipPanes
            ? ImGuiTabItemFlags_SetSelected
            : 0;

    if (ImGui::BeginTabItem("Available to download", nullptr, flags)) {
      selected_tab_ = 0;
      for (const auto& podcast : state_.podcasts()) {
        ImGui::PushID(podcast.podcast_uri().c_str());
        if (ImGui::TreeNodeEx(podcast.title().c_str(),
                              ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                  ImGuiTreeNodeFlags_Framed |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
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
      ImGui::EndTabItem();
    }

    flags = selected_tab_ == 0 && incoming_action.type == ActionType::kFlipPanes
                ? ImGuiTabItemFlags_SetSelected
                : 0;
    if (ImGui::BeginTabItem("Downloaded", nullptr, flags)) {
      selected_tab_ = 1;
      for (const auto& podcast : state_.podcasts()) {
        for (auto iter = podcast.episodes().rbegin();
             iter != podcast.episodes().rend(); ++iter) {
          const auto& episode = *iter;
          if (episode.download_status() == DownloadStatus::DOWNLOAD_SUCCESS) {
            action |= DrawEpisode(podcast, episode);
          }
        }
      }
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::SeparatorText("Help");
  ImGui::Button("About");
  ImGui::SameLine();
  ImGui::Button("Licenses");
  ImGui::SameLine();
  ImGui::Text("Service status: %s",
              service_status_ == ServiceStatus::kOnline ? "Online" : "Offline");
  ImGui::End();

  return action;
}

void PodcasterGui::UpdateServiceStatus(ServiceStatus status) {
  if (service_status_ != status) {
    if (status == ServiceStatus::kOnline) {
      state_ = client_.GetState();
    }
    service_status_ = status;
  }
}

void PodcasterGui::Run(const Action& incoming_action) {
  Action action = Draw(incoming_action);

  switch (action.type) {
    case ActionType::kRefresh:
      refresh_future_ =
          std::async(std::launch::async, [&] { return client_.Refresh(); });
      break;
    case ActionType::kDownloadEpisode:
      [[fallthrough]];
    case ActionType::kCancelDownload:
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

    ServiceStatus new_status = ServiceStatus::kOffline;
    if (auto episode_updates = client_.ReadUpdates(); episode_updates) {
      for (const auto& update : episode_updates.value()) {
        utils::ApplyUpdate(update, &state_);
      }
      new_status = ServiceStatus::kOnline;
    }
    UpdateServiceStatus(new_status);
  }
}

}  // namespace podcaster