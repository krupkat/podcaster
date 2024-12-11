#include <filesystem>
#include <future>
#include <iterator>
#include <mutex>
#include <signal.h>  // NOLINT(modernize-deprecated-headers)
#include <vector>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <google/protobuf/text_format.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/status_code_enum.h>
#include <pugixml.hpp>
#include <SDL_mixer.h>
#include <spdlog/spdlog.h>

#include "database.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include "sdl_mixer_utils.h"
#include "utils.h"

std::string DownloadFilename(const std::string& episode_uri) {
  auto last_slash = std::find(episode_uri.rbegin(), episode_uri.rend(), '/');
  if (last_slash == episode_uri.rend()) {
    return episode_uri;
  }
  int cut = std::distance(last_slash, episode_uri.rend());
  return episode_uri.substr(cut);
}

podcaster::Podcast DonwloadAndParseFeed(
    const std::string& feed_uri, const std::filesystem::path& cache_dir) {
  std::stringstream feed;

  try {
    curlpp::Easy my_request;
    my_request.setOpt<curlpp::options::Url>(feed_uri);
    my_request.setOpt<curlpp::options::WriteStream>(&feed);
#ifdef PODCASTER_HANDHELD_BUILD
    my_request.setOpt<curlpp::options::CaInfo>(
        "/etc/ssl/certs/ca-certificates.crt");
#endif
    my_request.perform();
  } catch (const std::exception& e) {
    spdlog::error("Failed to download feed: {}", e.what());
    return podcaster::Podcast();
  }

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(feed);

  if (!result) {
    spdlog::error("Failed to parse feed: {}", result.description());
    return podcaster::Podcast();
  }

  auto title = doc.select_node("/rss/channel/title").node();
  auto description = doc.select_node("/rss/channel/description").node();
  auto episodes = doc.select_nodes("/rss/channel/item");

  // sort in reverse document order
  episodes.sort(true);

  podcaster::Podcast podcast;
  podcast.set_podcast_uri(feed_uri);
  podcast.set_title(title.text().as_string());

  for (const auto& episode : episodes) {
    const auto& episode_node = episode.node();

    auto title = episode_node.select_node("title").node();
    auto description = episode_node.select_node("description").node();
    auto audio_uri =
        episode_node.select_node("enclosure").node().attribute("url");

    auto* episode_message = podcast.add_episodes();
    episode_message->set_title(title.text().as_string());
    episode_message->set_description(description.text().as_string());
    episode_message->set_episode_uri(audio_uri.as_string());
  }

  return podcast;
}

class PodcasterImpl final : public podcaster::Podcaster::Service {
 public:
  explicit PodcasterImpl(std::filesystem::path data_dir)
      : data_dir_(data_dir),
        db_(std::make_unique<podcaster::Database>(data_dir)),
        playback_controller_(this) {}

  grpc::Status State(grpc::ServerContext* context,
                     const podcaster::Empty* request,
                     podcaster::DatabaseState* response) override {
    auto state = db_->GetState();
    response->CopyFrom(state);
    return grpc::Status::OK;
  }

  grpc::Status Refresh(grpc::ServerContext* context,
                       const podcaster::Empty* request,
                       podcaster::DatabaseState* response) override {
    auto config_path = data_dir_ / "config.textproto";

    podcaster::Config config;

    if (std::filesystem::exists(config_path)) {
      std::ifstream config_file(config_path);
      if (config_file.is_open()) {
        google::protobuf::io::IstreamInputStream input_stream(&config_file);
        google::protobuf::TextFormat::Parse(&input_stream, &config);
      }
    }

    std::vector<podcaster::EpisodeUri> all_new_episodes;

    int idx = 1;
    for (const auto& feed : config.feed()) {
      auto podcast = DonwloadAndParseFeed(feed, data_dir_);
      auto new_episodes = db_->SavePodcast(podcast);
      std::move(new_episodes.begin(), new_episodes.end(),
                std::back_inserter(all_new_episodes));
    }

    db_->SaveState();
    auto state = db_->GetState();
    response->CopyFrom(state);

    for (const auto& uri : all_new_episodes) {
      podcaster::Empty response;
      Download(context, &uri, &response);
    }

    return grpc::Status::OK;
  }

  grpc::Status EpisodeUpdates(
      grpc::ServerContext* context, const podcaster::Empty* request,
      grpc::ServerWriter<podcaster::EpisodeUpdate>* response) override {
    playback_controller_.UpdatePlayback();
    std::lock_guard<std::mutex> lock(updates_mtx_);
    for (const auto& update : outbound_updates_) {
      response->Write(update);
    }
    outbound_updates_.clear();
    return grpc::Status::OK;
  }

  class XferInfoCallbackFunctor {
   public:
    XferInfoCallbackFunctor(PodcasterImpl* impl,
                            const podcaster::EpisodeUri& uri)
        : impl_(impl), uri_(uri) {}

    int Execute(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                curl_off_t ulnow) {
      if (dltotal == 0) {
        return 0;
      }
      float progress = static_cast<float>(dlnow) / dltotal;
      impl_->QueueDownloadProgress(uri_, progress);
      return 0;
    }

   private:
    PodcasterImpl* impl_;
    podcaster::EpisodeUri uri_;
  };

  static int XferInfoCallback(XferInfoCallbackFunctor* functor,
                              curl_off_t dltotal, curl_off_t dlnow,
                              curl_off_t ultotal, curl_off_t ulnow) {
    return functor->Execute(dltotal, dlnow, ultotal, ulnow);
  };

  struct Music {
    sdl::MixMusicPtr ptr;
    podcaster::EpisodeUri uri;
  };

  class PlaybackController {
   public:
    PlaybackController(PodcasterImpl* impl) : impl_(impl) {}
    ~PlaybackController() {
      if (music_) {
        Mix_HaltMusic();
      }
    }

    void Play(const podcaster::EpisodeUri& uri) {
      if (music_) {
        auto position = Mix_GetMusicPosition(music_->ptr.get());
        impl_->QueuePlaybackProgress(music_->uri, position * 1000.,
                                     QueueFlags::kPersist);
        impl_->QueuePlaybackStatus(music_->uri,
                                   podcaster::PlaybackStatus::NOT_PLAYING);
        Mix_HaltMusic();
        music_.reset();
      }

      if (auto episode = impl_->db_->FindEpisodeMutable(uri); episode) {
        std::filesystem::path download_path =
            impl_->data_dir_ / DownloadFilename(uri.episode_uri());

        if (auto music_ptr = sdl::LoadMusic(download_path); music_ptr) {
          music_ = {std::move(music_ptr), uri};

          Mix_PlayMusic(music_->ptr.get(), 0);

          impl_->QueuePlaybackStatus(uri, podcaster::PlaybackStatus::PLAYING);

          if (int elapsed_ms =
                  episode.value()->playback_progress().elapsed_ms();
              elapsed_ms > 0) {
            int res = Mix_SetMusicPosition(
                episode.value()->playback_progress().elapsed_ms() / 1000.);
          }

          if (int duration_ms = episode.value()->playback_progress().total_ms();
              duration_ms == 0) {
            auto duration = Mix_MusicDuration(music_->ptr.get());
            impl_->QueuePlaybackDuration(uri, duration * 1000.);
          }
        }
      }
    }

    void Pause(const podcaster::EpisodeUri& uri) {
      if (music_ and uri.podcast_uri() == music_->uri.podcast_uri() and
          uri.episode_uri() == music_->uri.episode_uri()) {
        auto position = Mix_GetMusicPosition(music_->ptr.get());
        impl_->QueuePlaybackProgress(music_->uri, position * 1000.,
                                     QueueFlags::kPersist);
        Mix_PauseMusic();
        impl_->QueuePlaybackStatus(music_->uri,
                                   podcaster::PlaybackStatus::PAUSED);
      }
    }

    void Resume(const podcaster::EpisodeUri& uri) {
      if (music_ and uri.podcast_uri() == music_->uri.podcast_uri() and
          uri.episode_uri() == music_->uri.episode_uri()) {
        Mix_ResumeMusic();
        impl_->QueuePlaybackStatus(music_->uri,
                                   podcaster::PlaybackStatus::PLAYING);
      }
    }

    void Stop(const podcaster::EpisodeUri& uri) {
      if (music_ and uri.podcast_uri() == music_->uri.podcast_uri() and
          uri.episode_uri() == music_->uri.episode_uri()) {
        impl_->QueuePlaybackProgress(music_->uri, 0, QueueFlags::kPersist);
        Mix_HaltMusic();
        impl_->QueuePlaybackStatus(music_->uri,
                                   podcaster::PlaybackStatus::NOT_PLAYING);
        music_.reset();
      }
    }

    void UpdatePlayback() {
      if (music_) {
        static int counter = 0;
        QueueFlags flags = QueueFlags::kTransient;
        if (counter++ % 60 == 0) {
          flags = QueueFlags::kPersist;
        }
        auto position = Mix_GetMusicPosition(music_->ptr.get());
        impl_->QueuePlaybackProgress(music_->uri, position * 1000., flags);
      }
    }

   private:
    std::optional<Music> music_;

    PodcasterImpl* impl_;
  };

  grpc::Status Play(grpc::ServerContext* context,
                    const podcaster::EpisodeUri* request,
                    podcaster::Empty* response) override {
    playback_controller_.Play(*request);
    return grpc::Status::OK;
  }

  grpc::Status Pause(grpc::ServerContext* context,
                     const podcaster::EpisodeUri* request,
                     podcaster::Empty* response) override {
    playback_controller_.Pause(*request);
    return grpc::Status::OK;
  }

  grpc::Status Resume(grpc::ServerContext* context,
                      const podcaster::EpisodeUri* request,
                      podcaster::Empty* response) override {
    playback_controller_.Resume(*request);
    return grpc::Status::OK;
  }

  grpc::Status Stop(grpc::ServerContext* context,
                    const podcaster::EpisodeUri* request,
                    podcaster::Empty* response) override {
    playback_controller_.Stop(*request);
    return grpc::Status::OK;
  }

  grpc::Status Delete(grpc::ServerContext* context,
                      const podcaster::EpisodeUri* request,
                      podcaster::Empty* response) override {
    if (auto episode = db_->FindEpisodeMutable(*request); episode) {
      if (episode.value()->download_status() ==
          podcaster::DownloadStatus::DOWNLOAD_SUCCESS) {
        std::filesystem::path download_path =
            data_dir_ / DownloadFilename(request->episode_uri());
        std::filesystem::remove(download_path);

        QueueDownloadProgress(*request, 0);
        QueueDownloadStatus(*request,
                            podcaster::DownloadStatus::NOT_DOWNLOADED);
        QueuePlaybackProgress(*request, 0, QueueFlags::kPersist);
        QueuePlaybackDuration(*request, 0);
      }
    }
    return grpc::Status::OK;
  }

  grpc::Status Download(grpc::ServerContext* context,
                        const podcaster::EpisodeUri* request,
                        podcaster::Empty* response) override {
    auto result_future = std::async(std::launch::async, [this, uri = *request] {
      QueueDownloadStatus(uri, podcaster::DownloadStatus::DOWNLOAD_IN_PROGRESS);

      std::filesystem::path download_path =
          data_dir_ / DownloadFilename(uri.episode_uri());
      std::ofstream download_file(download_path, std::ios::binary);

      if (not download_file.is_open()) {
        spdlog::error("Failed to open download file: {}",
                      download_path.string());
        QueueDownloadStatus(uri, podcaster::DownloadStatus::DOWNLOAD_ERROR);
        return;
      }

      try {
        curlpp::Easy my_request;
        my_request.setOpt<curlpp::options::Url>(uri.episode_uri());
        my_request.setOpt<curlpp::options::WriteStream>(&download_file);
#ifdef PODCASTER_HANDHELD_BUILD
        my_request.setOpt<curlpp::options::CaInfo>(
            "/etc/ssl/certs/ca-certificates.crt");
#endif

        XferInfoCallbackFunctor xfer_callback(this, uri);
        my_request.getCurlHandle().option(CURLOPT_XFERINFOFUNCTION,
                                          XferInfoCallback);
        my_request.getCurlHandle().option(CURLOPT_NOPROGRESS, 0L);
        my_request.getCurlHandle().option(CURLOPT_XFERINFODATA, &xfer_callback);
        my_request.perform();
      } catch (const std::exception& e) {
        spdlog::error("Failed to download episode: {}", e.what());
        QueueDownloadStatus(uri, podcaster::DownloadStatus::DOWNLOAD_ERROR);
        return;
      }

      QueueDownloadStatus(uri, podcaster::DownloadStatus::DOWNLOAD_SUCCESS);
    });

    {
      // cleanup completed downloads
      std::lock_guard<std::mutex> lock(download_mtx_);
      std::erase_if(downloads_in_progress_, [](auto& future) {
        if (utils::IsReady(future)) {
          future.get();  // may throw
          return true;
        }
        return false;
      });
      downloads_in_progress_.push_back(std::move(result_future));
    }

    return grpc::Status::OK;
  }

 private:
  enum class QueueFlags { kTransient, kPersist };
  void QueueUpdate(const podcaster::EpisodeUpdate& update, QueueFlags flags) {
    {
      std::lock_guard<std::mutex> lock(updates_mtx_);
      // cleanup superseded updates
      std::erase_if(outbound_updates_, [&update](const auto& u) {
        return u.uri().podcast_uri() == update.uri().podcast_uri() &&
               u.uri().episode_uri() == update.uri().episode_uri() &&
               u.status_case() == update.status_case();
      });
      outbound_updates_.push_back(update);
    }
    {
      std::lock_guard<std::mutex> lock(db_mutex_);
      db_->ApplyUpdate(update);
      if (flags == QueueFlags::kPersist) {
        db_->SaveState();
      }
    }
  }

  void QueueDownloadStatus(const podcaster::EpisodeUri& request,
                           podcaster::DownloadStatus status) {
    podcaster::EpisodeUpdate update;
    update.mutable_uri()->CopyFrom(request);
    update.set_new_download_status(status);
    QueueUpdate(update, QueueFlags::kPersist);
  }

  void QueueDownloadProgress(const podcaster::EpisodeUri& request,
                             float progress) {
    podcaster::EpisodeUpdate update;
    update.mutable_uri()->CopyFrom(request);
    update.set_new_download_progress(progress);
    QueueUpdate(update, QueueFlags::kTransient);
  }

  void QueuePlaybackStatus(const podcaster::EpisodeUri& request,
                           podcaster::PlaybackStatus status) {
    podcaster::EpisodeUpdate update;
    update.mutable_uri()->CopyFrom(request);
    update.set_new_playback_status(status);
    QueueUpdate(update, QueueFlags::kPersist);
  }

  void QueuePlaybackProgress(const podcaster::EpisodeUri& request,
                             int position_ms, QueueFlags flags) {
    podcaster::EpisodeUpdate update;
    update.mutable_uri()->CopyFrom(request);
    update.set_new_playback_progress(position_ms);
    QueueUpdate(update, flags);
  }

  void QueuePlaybackDuration(const podcaster::EpisodeUri& request,
                             int duration_ms) {
    podcaster::EpisodeUpdate update;
    update.mutable_uri()->CopyFrom(request);
    update.set_new_playback_duration(duration_ms);
    QueueUpdate(update, QueueFlags::kPersist);
  }

  std::filesystem::path data_dir_;

  std::mutex download_mtx_;
  std::vector<std::future<void>> downloads_in_progress_;

  std::mutex updates_mtx_;
  std::vector<podcaster::EpisodeUpdate> outbound_updates_;

  std::mutex db_mutex_;
  std::unique_ptr<podcaster::Database> db_;

  std::mutex playback_mtx_;
  PlaybackController playback_controller_;
};

std::atomic<int> cancel{0};

using SignalHandler = void (*)(int);

void CancelHandler(int /*signal*/) {
  cancel.fetch_add(1);
  cancel.notify_one();
}

void RegisterInterruptHandler(SignalHandler handler) {
  struct sigaction action;
  action.sa_handler = handler;
  sigfillset(&action.sa_mask);
  action.sa_flags = SA_RESETHAND;
  sigaction(SIGINT, &action, nullptr);
}

int main(int argc, char** argv) {
  curlpp::Cleanup cleanup;
  sdl::SDLMixerContext sdl_mixer_ctx = sdl::InitMix();
  RegisterInterruptHandler(CancelHandler);

  std::filesystem::path data_dir = argv[1];
  if (!data_dir.is_absolute()) {
    throw std::runtime_error("Data directory must be an absolute path.");
  }

  std::string server_address("0.0.0.0:50051");
  PodcasterImpl service(data_dir);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  cancel.wait(0);
  server->Shutdown();
}