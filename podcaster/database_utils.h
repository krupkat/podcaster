// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "podcaster/message.pb.h"

namespace podcaster::utils {

inline std::optional<Episode> FindEpisode(const EpisodeUri& uri,
                                          DatabaseState* state) {
  auto podcast =
      std::find_if(state->podcasts().begin(), state->podcasts().end(),
                   [&uri](const Podcast& p) {
                     return p.podcast_uri() == uri.podcast_uri();
                   });
  if (podcast == state->podcasts().end()) {
    return {};
  }

  auto episode =
      std::find_if(podcast->episodes().begin(), podcast->episodes().end(),
                   [&uri](const Episode& e) {
                     return e.episode_uri() == uri.episode_uri();
                   });

  if (episode == podcast->episodes().end()) {
    return {};
  }

  return *episode;
}

inline auto FindEpisodeMutable(const EpisodeUri& uri, DatabaseState* state)
    -> std::optional<google::protobuf::internal::RepeatedPtrIterator<Episode>> {
  auto podcast =
      std::find_if(state->mutable_podcasts()->begin(),
                   state->mutable_podcasts()->end(), [&uri](const Podcast& p) {
                     return p.podcast_uri() == uri.podcast_uri();
                   });
  if (podcast == state->mutable_podcasts()->end()) {
    return {};
  }

  auto episode = std::find_if(podcast->mutable_episodes()->begin(),
                              podcast->mutable_episodes()->end(),
                              [&uri](const Episode& e) {
                                return e.episode_uri() == uri.episode_uri();
                              });

  if (episode == podcast->mutable_episodes()->end()) {
    return {};
  }

  return episode;
}

inline void ApplyUpdate(const EpisodeUpdate& update, DatabaseState* state) {
  auto episode = FindEpisodeMutable(update.uri(), state);
  if (not episode) {
    return;
  }

  switch (update.status_case()) {
    case EpisodeUpdate::StatusCase::kNewDownloadStatus:
      episode.value()->set_download_status(update.new_download_status());
      break;
    case EpisodeUpdate::StatusCase::kNewDownloadProgress:
      episode.value()->mutable_download_progress()->CopyFrom(
          update.new_download_progress());
      break;
    case EpisodeUpdate::StatusCase::kNewPlaybackStatus:
      episode.value()->set_playback_status(update.new_playback_status());
      break;
    case EpisodeUpdate::StatusCase::kNewPlaybackProgress:
      episode.value()->mutable_playback_progress()->set_elapsed_ms(
          update.new_playback_progress());
      break;
    case EpisodeUpdate::StatusCase::kNewPlaybackDuration:
      episode.value()->mutable_playback_progress()->set_total_ms(
          update.new_playback_duration());
    default:
      break;
  }
}

}  // namespace podcaster::utils