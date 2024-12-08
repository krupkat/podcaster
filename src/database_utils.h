#pragma once

#include "message.pb.h"

namespace podcaster::utils {

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
      episode.value()->set_download_progress(update.new_download_progress());
      break;
    default:
      break;
  }
}

}  // namespace podcaster