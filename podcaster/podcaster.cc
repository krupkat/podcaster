#include <filesystem>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <signal.h>  // NOLINT(modernize-deprecated-headers)
#include <sstream>
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

#include "podcaster/database.h"
#include "podcaster/message.grpc.pb.h"
#include "podcaster/message.pb.h"
#include "podcaster/sdl_mixer_utils.h"
#include "podcaster/tidy_utils.h"
#include "podcaster/utils.h"

constexpr int kMaxEpisodesPerPodcast = 10;
constexpr int kPreviewLength = 200;

std::atomic<int> cancel_service{0};

void CancelHandler(int /*signal*/) {
  cancel_service.fetch_add(1);
  cancel_service.notify_one();
}

std::string DownloadFilename(const std::string& episode_uri) {
  auto last_slash = std::find(episode_uri.rbegin(), episode_uri.rend(), '/');
  if (last_slash == episode_uri.rend()) {
    return episode_uri;
  }
  int cut = std::distance(last_slash, episode_uri.rend());
  return episode_uri.substr(cut);
}

class HTMLStripWalker : public pugi::xml_tree_walker {
 public:
  HTMLStripWalker() {}

  bool for_each(pugi::xml_node& node) override {
    if (node.type() == pugi::node_pcdata) {
      Save(node.value());
    }
    if (node.type() == pugi::node_element) {
      if (std::string name = node.name(); name == "p" || name == "br") {
        NewLine();
      }
    }
    return true;
  }

  bool end(pugi::xml_node& node) override {
    if (character_count_ > kPreviewLength) {
      result_short_ << "...";
    }
    return true;
  }

  std::string ResultShort() const { return result_short_.str(); }
  std::string ResultLong() const { return result_long_.str(); }

 private:
  void Save(const char* str) {
    for (const char* c = str; *c != 0; ++c) {
      Put(*c);
    }
  }

  void NewLine() { Put('\n'); }

  void Put(char c) {
    // skip leading whitespace
    if (first_char_) {
      if (c == ' ' || c == '\n') {
        return;
      }
      first_char_ = false;
    }

    // max 2 consecutive newlines
    if (c == '\n') {
      consecutive_newlines_++;
      if (consecutive_newlines_ > 2) {
        return;
      }
    } else {
      consecutive_newlines_ = 0;
    }

    // first kPreviewLength characters are short description
    if (character_count_ < kPreviewLength) {
      result_short_ << c;
    } else {
      result_long_ << c;
    }
    character_count_++;
  }

  bool first_char_ = true;
  int character_count_ = 0;
  int consecutive_newlines_ = 0;

  std::stringstream result_short_;
  std::stringstream result_long_;
};

struct ParsedDescription {
  std::string short_description;
  std::string long_description;
};

ParsedDescription ParseDescription(const std::string& input) {
  auto xhtml = tidy::ConvertToXHTML(input);

  pugi::xml_document doc;
  pugi::xml_parse_result xml_result = doc.load_buffer_inplace(
      xhtml.data(), xhtml.size(), pugi::parse_default, pugi::encoding_auto);

  if (!xml_result) {
    spdlog::error("Failed to parse XHTML: {}", xml_result.description());
    return {input};
  }

  HTMLStripWalker walker;
  doc.traverse(walker);
  return {walker.ResultShort(), walker.ResultLong()};
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

  const auto* first =
      std::max(episodes.begin(), episodes.end() - kMaxEpisodesPerPodcast);
  for (const auto* iter = first; iter != episodes.end(); iter++) {
    const auto& episode = *iter;
    const auto& episode_node = episode.node();

    auto title = episode_node.select_node("title").node();
    auto description = episode_node.select_node("description").node();
    auto audio_uri =
        episode_node.select_node("enclosure").node().attribute("url");

    auto parsed_description = ParseDescription(description.text().as_string());

    auto* episode_message = podcast.add_episodes();
    episode_message->set_title(title.text().as_string());
    episode_message->set_description_short(
        parsed_description.short_description);
    episode_message->set_description_long(parsed_description.long_description);
    episode_message->set_episode_uri(audio_uri.as_string());
  }

  return podcast;
}

struct ActiveDownload {
  podcaster::EpisodeUri uri;
  std::unique_ptr<std::atomic_bool> cancel;
  std::future<void> future;
};

class PodcasterImpl final : public podcaster::Podcaster::Service {
 public:
  explicit PodcasterImpl(std::filesystem::path data_dir)
      : data_dir_(data_dir),
        db_(std::make_unique<podcaster::Database>(data_dir)),
        playback_controller_(this) {}

  ~PodcasterImpl() {
    std::lock_guard<std::mutex> lock(download_mtx_);
    for (auto& download : downloads_in_progress_) {
      download.cancel->store(true);
      // ignore exceptions
      download.future.wait();
    }
  }

  PodcasterImpl(const PodcasterImpl&) = delete;
  PodcasterImpl& operator=(const PodcasterImpl&) = delete;
  PodcasterImpl(PodcasterImpl&&) = delete;
  PodcasterImpl& operator=(PodcasterImpl&&) = delete;

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
                            const podcaster::EpisodeUri& uri,
                            std::atomic_bool* cancel)
        : impl_(impl), uri_(uri), cancel_(cancel) {}

    int Execute(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                curl_off_t ulnow) {
      if (cancel_->load()) {
        return 1;
      }
      if (dltotal == 0) {
        return 0;
      }
      impl_->QueueDownloadProgress(uri_, dlnow, dltotal);
      return 0;
    }

   private:
    PodcasterImpl* impl_;
    podcaster::EpisodeUri uri_;
    std::atomic_bool* cancel_;
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
        impl_->QueuePlaybackStatus(music_->uri,
                                   podcaster::PlaybackStatus::NOT_PLAYING);
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
      if (not music_) {
        // recovery, state was paused when service shut down
        Play(uri);
      } else if (uri.podcast_uri() == music_->uri.podcast_uri() and
                 uri.episode_uri() == music_->uri.episode_uri()) {
        Mix_ResumeMusic();
        impl_->QueuePlaybackStatus(music_->uri,
                                   podcaster::PlaybackStatus::PLAYING);
      } else {
        // recovery, state was paused when service shut down
        // other episode is playing
        Play(uri);
      }
    }

    void Stop(const podcaster::EpisodeUri& uri) {
      if (not music_) {
        // recovery, state was playing when service shut down
        impl_->QueuePlaybackStatus(uri, podcaster::PlaybackStatus::NOT_PLAYING);
      } else if (uri.podcast_uri() == music_->uri.podcast_uri() and
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

    bool IsPlaying() const {
      return music_ and Mix_PlayingMusic() == 1 and Mix_PausedMusic() == 0;
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

  grpc::Status ShutdownIfNotPlaying(grpc::ServerContext* context,
                                    const podcaster::Empty* request,
                                    podcaster::Empty* response) override {
    if (not playback_controller_.IsPlaying()) {
      CancelHandler(0);
    }
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

        QueueDownloadProgress(*request, 0, 0);
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
    auto cancel_flag = std::make_unique<std::atomic_bool>(false);

    auto result_future = std::async(
        std::launch::async,
        [this, uri = *request, cancel_ptr = cancel_flag.get()] {
          QueueDownloadStatus(uri,
                              podcaster::DownloadStatus::DOWNLOAD_IN_PROGRESS);

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
            my_request.setOpt<curlpp::options::FollowLocation>(true);

            spdlog::info("Downloading: {}", uri.episode_uri());
#ifdef PODCASTER_HANDHELD_BUILD
            my_request.setOpt<curlpp::options::CaInfo>(
                "/etc/ssl/certs/ca-certificates.crt");
#endif

            XferInfoCallbackFunctor xfer_callback(this, uri, cancel_ptr);
            my_request.getCurlHandle().option(CURLOPT_XFERINFOFUNCTION,
                                              XferInfoCallback);
            my_request.getCurlHandle().option(CURLOPT_NOPROGRESS, 0L);
            my_request.getCurlHandle().option(CURLOPT_XFERINFODATA,
                                              &xfer_callback);
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
      std::erase_if(downloads_in_progress_, [](auto& download) {
        if (utils::IsReady(download.future)) {
          download.future.get();  // may throw
          return true;
        }
        return false;
      });
      downloads_in_progress_.push_back(
          ActiveDownload{.uri = *request,
                         .cancel = std::move(cancel_flag),
                         .future = std::move(result_future)});
    }

    return grpc::Status::OK;
  }

  grpc::Status CancelDownload(grpc::ServerContext* context,
                              const podcaster::EpisodeUri* request,
                              podcaster::Empty* response) override {
    bool live_cancel = false;

    {
      std::lock_guard<std::mutex> lock(download_mtx_);
      for (auto& download : downloads_in_progress_) {
        if (download.uri.podcast_uri() == request->podcast_uri() and
            download.uri.episode_uri() == request->episode_uri()) {
          download.cancel->store(true);
          live_cancel = true;
        }
      }
    }

    if (not live_cancel) {
      // cleanup in case of unclean shutdown
      QueueDownloadStatus(*request, podcaster::DownloadStatus::NOT_DOWNLOADED);
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
                             float progress, float size) {
    podcaster::EpisodeUpdate update;
    update.mutable_uri()->CopyFrom(request);
    auto* download_progress = update.mutable_new_download_progress();
    download_progress->set_downloaded_bytes(progress);
    download_progress->set_total_bytes(size);
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
  std::vector<ActiveDownload> downloads_in_progress_;

  std::mutex updates_mtx_;
  std::vector<podcaster::EpisodeUpdate> outbound_updates_;

  std::mutex db_mutex_;
  std::unique_ptr<podcaster::Database> db_;

  std::mutex playback_mtx_;
  PlaybackController playback_controller_;
};

using SignalHandler = void (*)(int);

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
  spdlog::info("Server listening on {}", server_address);

  cancel_service.wait(0);
  server->Shutdown();
}