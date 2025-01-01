#include "podcaster/podcaster_impl.h"

#include <google/protobuf/text_format.h>

#include "podcaster/tidy_utils.h"
#include "podcaster/utils.h"
#include "podcaster/xml_utils.h"

namespace podcaster {

constexpr int kMaxEpisodesPerPodcast = 10;

podcaster::Config LoadConfig(const std::filesystem::path& data_dir) {
  auto config_path = data_dir / "config.textproto";

  podcaster::Config config;

  if (not std::filesystem::exists(data_dir)) {
    std::filesystem::create_directories(data_dir);
  }

  if (not std::filesystem::exists(config_path)) {
    std::ofstream config_file(config_path);
    if (config_file.is_open()) {
      config_file <<
          R"(# Specify the feeds you want to subscribe to, one feed per line:
# feed: "http://www.2600.com/oth-broadband.xml"
# feed: "https://podcast.darknetdiaries.com/"
)";
    }
  }

  std::ifstream config_file(config_path);
  if (config_file.is_open()) {
    google::protobuf::io::IstreamInputStream input_stream(&config_file);
    google::protobuf::TextFormat::Parse(&input_stream, &config);
  }

  return config;
}

std::string DownloadFilename(const std::string& episode_uri) {
  auto last_slash = std::find(episode_uri.rbegin(), episode_uri.rend(), '/');
  if (last_slash == episode_uri.rend()) {
    return episode_uri;
  }
  int cut = std::distance(last_slash, episode_uri.rend());
  return episode_uri.substr(cut);
}

int XferInfoCallbackFunctor::Execute(curl_off_t dltotal, curl_off_t dlnow,
                                     curl_off_t ultotal, curl_off_t ulnow) {
  if (cancel_->load()) {
    return 1;
  }
  if (dltotal == 0) {
    return 0;
  }
  impl_->QueueDownloadProgress(uri_, dlnow, dltotal, QueueFlags::kTransient);
  return 0;
}

int XferInfoCallback(XferInfoCallbackFunctor* functor, curl_off_t dltotal,
                     curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
  return functor->Execute(dltotal, dlnow, ultotal, ulnow);
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

  xml::XHTMLStripWalker walker;
  doc.traverse(walker);
  return {walker.ResultShort(), walker.ResultLong()};
}

std::optional<podcaster::Podcast> DonwloadAndParseFeed(
    const std::string& feed_uri, const std::filesystem::path& cache_dir) {
  std::stringstream feed;

  try {
    curlpp::Easy my_request;
    my_request.setOpt<curlpp::options::Url>(feed_uri);
    my_request.setOpt<curlpp::options::WriteStream>(&feed);
    my_request.setOpt<curlpp::options::FollowLocation>(true);
#ifdef PODCASTER_HANDHELD_BUILD
    my_request.setOpt<curlpp::options::CaInfo>(
        "/etc/ssl/certs/ca-certificates.crt");
#endif
    my_request.perform();
  } catch (const std::exception& e) {
    spdlog::error("Failed to download feed: {}", e.what());
    return {};
  }

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load(feed);

  if (!result) {
    spdlog::error("Failed to parse feed: {}", result.description());
    return {};
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

PlaybackController::~PlaybackController() {
  if (music_) {
    Mix_HaltMusic();
    impl_->QueuePlaybackStatus(music_->uri,
                               podcaster::PlaybackStatus::NOT_PLAYING);
  }
}

void PlaybackController::Play(const podcaster::EpisodeUri& uri) {
  if (music_) {
    auto position = Mix_GetMusicPosition(music_->ptr.get());
    impl_->QueuePlaybackProgress(music_->uri, position * 1000);
    impl_->QueuePlaybackStatus(music_->uri,
                               podcaster::PlaybackStatus::NOT_PLAYING);
    Mix_HaltMusic();
    music_.reset();
  }

  if (auto episode = impl_->db_->FindEpisode(uri); episode) {
    std::filesystem::path download_path =
        impl_->data_dir_ / DownloadFilename(uri.episode_uri());

    if (auto music_ptr = sdl::LoadMusic(download_path); music_ptr) {
      music_ = {std::move(music_ptr), uri};

      Mix_PlayMusic(music_->ptr.get(), 0);

      impl_->QueuePlaybackStatus(uri, podcaster::PlaybackStatus::PLAYING);

      if (int elapsed_ms = episode.value().playback_progress().elapsed_ms();
          elapsed_ms > 0) {
        int res = Mix_SetMusicPosition(
            episode.value().playback_progress().elapsed_ms() / 1000.);
      }

      if (int duration_ms = episode.value().playback_progress().total_ms();
          duration_ms == 0) {
        auto duration = Mix_MusicDuration(music_->ptr.get());
        impl_->QueuePlaybackDuration(uri, duration * 1000.);
      }
    }
  }
}

void PlaybackController::Pause(const podcaster::EpisodeUri& uri) {
  if (music_ and uri.podcast_uri() == music_->uri.podcast_uri() and
      uri.episode_uri() == music_->uri.episode_uri()) {
    auto position = Mix_GetMusicPosition(music_->ptr.get());
    impl_->QueuePlaybackProgress(music_->uri, position * 1000.);
    Mix_PauseMusic();
    impl_->QueuePlaybackStatus(music_->uri, podcaster::PlaybackStatus::PAUSED);
  }
}

void PlaybackController::Resume(const podcaster::EpisodeUri& uri) {
  if (not music_) {
    // recovery, state was paused when service shut down
    Play(uri);
  } else if (uri.podcast_uri() == music_->uri.podcast_uri() and
             uri.episode_uri() == music_->uri.episode_uri()) {
    Mix_ResumeMusic();
    impl_->QueuePlaybackStatus(music_->uri, podcaster::PlaybackStatus::PLAYING);
  } else {
    // recovery, state was paused when service shut down
    // other episode is playing
    Play(uri);
  }
}

void PlaybackController::Stop(const podcaster::EpisodeUri& uri) {
  if (not music_) {
    // recovery, state was playing when service shut down
    impl_->QueuePlaybackStatus(uri, podcaster::PlaybackStatus::NOT_PLAYING);
  } else if (uri.podcast_uri() == music_->uri.podcast_uri() and
             uri.episode_uri() == music_->uri.episode_uri()) {
    impl_->QueuePlaybackProgress(music_->uri, 0);
    Mix_HaltMusic();
    impl_->QueuePlaybackStatus(music_->uri,
                               podcaster::PlaybackStatus::NOT_PLAYING);
    music_.reset();
  }
}

void PlaybackController::UpdatePlayback() {
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

bool PlaybackController::IsPlaying() const {
  return music_ and Mix_PlayingMusic() == 1 and Mix_PausedMusic() == 0;
}

PodcasterImpl::PodcasterImpl(std::filesystem::path data_dir,
                             std::function<void()> shutdown_callback)
    : data_dir_(data_dir),
      shutdown_callback_(shutdown_callback),
      db_(std::make_unique<podcaster::Database>(data_dir)),
      playback_controller_(this) {}

PodcasterImpl::~PodcasterImpl() {
  std::lock_guard<std::mutex> lock(download_mtx_);
  for (auto& download : downloads_in_progress_) {
    download.cancel->store(true);
    // ignore exceptions
    download.future.wait();
  }
}

grpc::Status PodcasterImpl::State(grpc::ServerContext* context,
                                  const podcaster::Empty* request,
                                  podcaster::DatabaseState* response) {
  auto state = db_->GetState();
  response->CopyFrom(state);
  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::Refresh(grpc::ServerContext* context,
                                    const podcaster::Empty* request,
                                    podcaster::DatabaseState* response) {
  auto config = LoadConfig(data_dir_);

  std::vector<podcaster::EpisodeUri> all_new_episodes;

  for (const auto& feed : config.feed()) {
    if (auto podcast = DonwloadAndParseFeed(feed, data_dir_)) {
      std::lock_guard<std::mutex> lock(db_mutex_);
      auto new_episodes = db_->SavePodcast(podcast.value());
      std::move(new_episodes.begin(), new_episodes.end(),
                std::back_inserter(all_new_episodes));
    }
  }

  {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_->SaveState();
  }
  auto state = db_->GetState();
  response->CopyFrom(state);

  for (const auto& uri : all_new_episodes) {
    podcaster::Empty response;
    Download(context, &uri, &response);
  }

  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::EpisodeUpdates(
    grpc::ServerContext* context, const podcaster::Empty* request,
    grpc::ServerWriter<podcaster::EpisodeUpdate>* response) {
  playback_controller_.UpdatePlayback();
  std::lock_guard<std::mutex> lock(updates_mtx_);
  for (const auto& update : outbound_updates_) {
    response->Write(update);
  }
  outbound_updates_.clear();
  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::Play(grpc::ServerContext* context,
                                 const podcaster::EpisodeUri* request,
                                 podcaster::Empty* response) {
  playback_controller_.Play(*request);
  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::Pause(grpc::ServerContext* context,
                                  const podcaster::EpisodeUri* request,
                                  podcaster::Empty* response) {
  playback_controller_.Pause(*request);
  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::Resume(grpc::ServerContext* context,
                                   const podcaster::EpisodeUri* request,
                                   podcaster::Empty* response) {
  playback_controller_.Resume(*request);
  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::Stop(grpc::ServerContext* context,
                                 const podcaster::EpisodeUri* request,
                                 podcaster::Empty* response) {
  playback_controller_.Stop(*request);
  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::ShutdownIfNotPlaying(
    grpc::ServerContext* context, const podcaster::Empty* request,
    podcaster::Empty* response) {
  if (not playback_controller_.IsPlaying()) {
    shutdown_callback_();
  }
  return grpc::Status::OK;
}

void PodcasterImpl::DeleteImpl(const podcaster::EpisodeUri& uri,
                               QueueFlags flags) {
  std::filesystem::path download_path =
      data_dir_ / DownloadFilename(uri.episode_uri());
  std::filesystem::remove(download_path);

  QueueDownloadProgress(uri, 0, 0, flags);
  QueueDownloadStatus(uri, podcaster::DownloadStatus::NOT_DOWNLOADED, flags);
  QueuePlaybackProgress(uri, 0, flags);
  QueuePlaybackDuration(uri, 0, flags);

  if (flags == QueueFlags::kPersist) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_->SaveState();
  }
}

grpc::Status PodcasterImpl::CleanupDownloads(grpc::ServerContext* context,
                                             const podcaster::Empty* request,
                                             podcaster::Empty* response) {
  spdlog::info("CleanupDownloads");
  auto state = db_->GetState();
  podcaster::EpisodeUri uri;
  for (const auto& podcast : state.podcasts()) {
    uri.set_podcast_uri(podcast.podcast_uri());
    for (const auto& episode : podcast.episodes()) {
      uri.set_episode_uri(episode.episode_uri());

      if ((episode.download_status() ==
               podcaster::DownloadStatus::DOWNLOAD_SUCCESS or
           episode.download_status() ==
               podcaster::DownloadStatus::DOWNLOAD_ERROR) and
          episode.playback_status() != podcaster::PlaybackStatus::PLAYING) {
        // write to disk later in bulk with SaveState
        DeleteImpl(uri, QueueFlags::kTransient);
      }
    }
  }

  {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_->SaveState();
  }

  // iterate mp3 files in data_dir_
  for (const auto& entry : std::filesystem::directory_iterator(data_dir_)) {
    if (entry.path().extension() == ".mp3") {
      spdlog::warn("Found orphaned file: {}, deleting", entry.path().string());
      std::filesystem::remove(entry.path());
    }
  }

  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::CleanupAll(grpc::ServerContext* context,
                                       const podcaster::Empty* request,
                                       podcaster::Empty* response) {
  // stop playback
  playback_controller_ = PlaybackController{this};

  // stop downloads
  {
    std::lock_guard<std::mutex> lock(download_mtx_);
    for (auto& download : downloads_in_progress_) {
      download.cancel->store(true);
      // swallow exceptions
      download.future.wait();
    }
  }

  // recreate database
  {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_.reset();

    for (const auto& entry : std::filesystem::directory_iterator(data_dir_)) {
      if (entry.path().filename() != "config.textproto") {
        std::filesystem::remove_all(entry.path());
      }
    }

    db_ = std::make_unique<podcaster::Database>(data_dir_);
  }
  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::Delete(grpc::ServerContext* context,
                                   const podcaster::EpisodeUri* request,
                                   podcaster::Empty* response) {
  if (auto episode = db_->FindEpisode(*request); episode) {
    if (episode.value().download_status() ==
        podcaster::DownloadStatus::DOWNLOAD_SUCCESS) {
      DeleteImpl(*request, QueueFlags::kPersist);
    }
  }
  return grpc::Status::OK;
}

grpc::Status PodcasterImpl::Download(grpc::ServerContext* context,
                                     const podcaster::EpisodeUri* request,
                                     podcaster::Empty* response) {
  auto cancel_flag = std::make_unique<std::atomic_bool>(false);

  auto result_future = std::async(std::launch::async, [this, uri = *request,
                                                       cancel_ptr =
                                                           cancel_flag.get()] {
    QueueDownloadStatus(uri, podcaster::DownloadStatus::DOWNLOAD_IN_PROGRESS);

    std::filesystem::path download_path =
        data_dir_ / DownloadFilename(uri.episode_uri());
    std::ofstream download_file(download_path, std::ios::binary);

    if (not download_file.is_open()) {
      spdlog::error("Failed to open download file: {}", download_path.string());
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
    std::erase_if(downloads_in_progress_, [](auto& download) {
      if (::utils::IsReady(download.future)) {
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

grpc::Status PodcasterImpl::CancelDownload(grpc::ServerContext* context,
                                           const podcaster::EpisodeUri* request,
                                           podcaster::Empty* response) {
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

grpc::Status PodcasterImpl::GetConfigInfo(grpc::ServerContext* context,
                                          const podcaster::Empty* request,
                                          podcaster::ConfigInfo* response) {
  auto config = LoadConfig(data_dir_);
  response->set_config_path(data_dir_ / "config.textproto");
  response->mutable_config()->CopyFrom(config);
  return grpc::Status::OK;
}

void PodcasterImpl::QueueUpdate(const podcaster::EpisodeUpdate& update,
                                QueueFlags flags) {
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

void PodcasterImpl::QueueDownloadStatus(const podcaster::EpisodeUri& request,
                                        podcaster::DownloadStatus status,
                                        QueueFlags flags) {
  podcaster::EpisodeUpdate update;
  update.mutable_uri()->CopyFrom(request);
  update.set_new_download_status(status);
  QueueUpdate(update, flags);
}

void PodcasterImpl::QueueDownloadProgress(const podcaster::EpisodeUri& request,
                                          float progress, float size,
                                          QueueFlags flags) {
  podcaster::EpisodeUpdate update;
  update.mutable_uri()->CopyFrom(request);
  auto* download_progress = update.mutable_new_download_progress();
  download_progress->set_downloaded_bytes(progress);
  download_progress->set_total_bytes(size);
  QueueUpdate(update, flags);
}

void PodcasterImpl::QueuePlaybackStatus(const podcaster::EpisodeUri& request,
                                        podcaster::PlaybackStatus status,
                                        QueueFlags flags) {
  podcaster::EpisodeUpdate update;
  update.mutable_uri()->CopyFrom(request);
  update.set_new_playback_status(status);
  QueueUpdate(update, flags);
}

void PodcasterImpl::QueuePlaybackProgress(const podcaster::EpisodeUri& request,
                                          int position_ms, QueueFlags flags) {
  podcaster::EpisodeUpdate update;
  update.mutable_uri()->CopyFrom(request);
  update.set_new_playback_progress(position_ms);
  QueueUpdate(update, flags);
}

void PodcasterImpl::QueuePlaybackDuration(const podcaster::EpisodeUri& request,
                                          int duration_ms, QueueFlags flags) {
  podcaster::EpisodeUpdate update;
  update.mutable_uri()->CopyFrom(request);
  update.set_new_playback_duration(duration_ms);
  QueueUpdate(update, flags);
}

}  // namespace podcaster