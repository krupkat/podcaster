#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "message.pb.h"

namespace podcaster {
class Database {
 public:
  explicit Database(std::filesystem::path data_dir) : data_dir_(data_dir) {
    std::filesystem::path file_path = data_dir_ / "db.bin";
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
      return;
    }
    db_.ParseFromIstream(&file);
  }

  const DatabaseState& GetState() const { return db_; }

  void SaveState() {
    std::filesystem::path file_path = data_dir_ / "db.bin";
    std::ofstream file(file_path, std::ios::binary);
    db_.SerializeToOstream(&file);
  }

  template <typename TEpisodeList>
  bool UpdateEpisode(TEpisodeList* episode_list,
                     const Episode& updated_episode) {
    auto episode =
        std::find_if(episode_list->begin(), episode_list->end(),
                     [&updated_episode](const Episode& e) {
                       return e.episode_uri() == updated_episode.episode_uri();
                     });

    if (episode != episode_list->end()) {
      episode->MergeFrom(updated_episode);
      return true;
    }

    return false;
  }

  void SavePodcast(const Podcast& updated_podcast) {
    auto podcast = std::find_if(
        db_.mutable_podcasts()->begin(), db_.mutable_podcasts()->end(),
        [&updated_podcast](const Podcast& p) {
          return p.podcast_uri() == updated_podcast.podcast_uri();
        });

    if (podcast != db_.mutable_podcasts()->end()) {
      // existing podcast
      for (const auto& updated_episode : updated_podcast.episodes()) {
        if (UpdateEpisode(podcast->mutable_episodes(), updated_episode)) {
          // existing episode, update queue
          UpdateEpisode(db_.mutable_queue(), updated_episode);
        } else {
          // new episode
          podcast->add_episodes()->CopyFrom(updated_episode);
          // add to queue
          db_.add_queue()->CopyFrom(updated_episode);
        }
      }
    } else {
      // new podcast
      db_.add_podcasts()->CopyFrom(updated_podcast);

      int latest_episode_index = updated_podcast.episodes_size() - 1;
      if (latest_episode_index >= 0) {
        db_.add_queue()->CopyFrom(
            updated_podcast.episodes().at(latest_episode_index));
      }
    }
  }

 private:
  std::filesystem::path data_dir_;
  DatabaseState db_;
};
}  // namespace podcaster