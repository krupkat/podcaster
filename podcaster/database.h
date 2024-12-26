#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "podcaster/database_utils.h"
#include "podcaster/message.pb.h"

namespace podcaster {
class Database {
 public:
  explicit Database(std::filesystem::path data_dir) : data_dir_(data_dir) {
    std::filesystem::path file_path = data_dir_ / "db.bin";
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
      db_.set_db_path(data_dir_);
      db_.set_config_path(data_dir_ / "config.textproto");
      return;
    }
    db_.ParseFromIstream(&file);
  }

  ~Database() { SaveState(); }

  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;
  Database(Database&&) = delete;
  Database& operator=(Database&&) = delete;

  const DatabaseState& GetState() const { return db_; }

  auto FindEpisodeMutable(const EpisodeUri& uri)
      -> std::optional<
          google::protobuf::internal::RepeatedPtrIterator<Episode>> {
    return utils::FindEpisodeMutable(uri, &db_);
  }

  void ApplyUpdate(const EpisodeUpdate& update) {
    utils::ApplyUpdate(update, &db_);
  }

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

  std::vector<EpisodeUri> SavePodcast(const Podcast& updated_podcast) {
    auto podcast = std::find_if(
        db_.mutable_podcasts()->begin(), db_.mutable_podcasts()->end(),
        [&updated_podcast](const Podcast& p) {
          return p.podcast_uri() == updated_podcast.podcast_uri();
        });

    std::vector<EpisodeUri> new_episodes;

    if (podcast != db_.mutable_podcasts()->end()) {
      // existing podcast
      for (const auto& updated_episode : updated_podcast.episodes()) {
        if (UpdateEpisode(podcast->mutable_episodes(), updated_episode)) {
          // existing episode was updated
        } else {
          // new episode
          podcast->add_episodes()->CopyFrom(updated_episode);
          // queue download
          EpisodeUri uri;
          uri.set_podcast_uri(podcast->podcast_uri());
          uri.set_episode_uri(updated_episode.episode_uri());
          new_episodes.push_back(uri);
        }
      }
    } else {
      // new podcast
      db_.add_podcasts()->CopyFrom(updated_podcast);

      int latest_episode_index = updated_podcast.episodes_size() - 1;
      if (latest_episode_index >= 0) {
        EpisodeUri uri;
        uri.set_podcast_uri(updated_podcast.podcast_uri());
        uri.set_episode_uri(
            updated_podcast.episodes().at(latest_episode_index).episode_uri());
        new_episodes.push_back(uri);
      }
    }

    return new_episodes;
  }

 private:
  std::filesystem::path data_dir_;
  DatabaseState db_;
};
}  // namespace podcaster