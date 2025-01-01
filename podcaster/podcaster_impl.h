#pragma once

#include <future>
#include <memory>
#include <vector>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include "podcaster/database.h"
#include "podcaster/message.grpc.pb.h"
#include "podcaster/message.pb.h"
#include "podcaster/sdl_mixer_utils.h"

namespace podcaster {

struct ParsedDescription {
  std::string short_description;
  std::string long_description;
};

ParsedDescription ParseDescription(const std::string& input);

std::string DownloadFilename(const std::string& episode_uri);

enum class QueueFlags { kTransient, kPersist };

struct ActiveDownload {
  podcaster::EpisodeUri uri;
  std::unique_ptr<std::atomic_bool> cancel;
  std::future<void> future;
};

class PodcasterImpl;

class XferInfoCallbackFunctor {
 public:
  XferInfoCallbackFunctor(PodcasterImpl* impl, const podcaster::EpisodeUri& uri,
                          std::atomic_bool* cancel)
      : impl_(impl), uri_(uri), cancel_(cancel) {}

  int Execute(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
              curl_off_t ulnow);

 private:
  PodcasterImpl* impl_;
  podcaster::EpisodeUri uri_;
  std::atomic_bool* cancel_;
};

struct Music {
  sdl::MixMusicPtr ptr;
  podcaster::EpisodeUri uri;
};

class PlaybackController {
 public:
  PlaybackController(PodcasterImpl* impl) : impl_(impl) {}
  ~PlaybackController();
  PlaybackController(PlaybackController&&) = default;
  PlaybackController& operator=(PlaybackController&&) = default;
  PlaybackController(const PlaybackController&) = delete;
  PlaybackController& operator=(const PlaybackController&) = delete;

  void Play(const podcaster::EpisodeUri& uri);

  void Pause(const podcaster::EpisodeUri& uri);

  void Resume(const podcaster::EpisodeUri& uri);

  void Stop(const podcaster::EpisodeUri& uri);

  void UpdatePlayback();

  bool IsPlaying() const;

 private:
  std::optional<Music> music_;

  PodcasterImpl* impl_;
};

class PodcasterImpl final : public podcaster::Podcaster::Service {
 public:
  explicit PodcasterImpl(std::filesystem::path data_dir,
                         std::function<void()> shutdown_callback);
  ~PodcasterImpl();
  PodcasterImpl(const PodcasterImpl&) = delete;
  PodcasterImpl& operator=(const PodcasterImpl&) = delete;
  PodcasterImpl(PodcasterImpl&&) = delete;
  PodcasterImpl& operator=(PodcasterImpl&&) = delete;

  grpc::Status State(grpc::ServerContext* context,
                     const podcaster::Empty* request,
                     podcaster::DatabaseState* response) override;

  grpc::Status Refresh(grpc::ServerContext* context,
                       const podcaster::Empty* request,
                       podcaster::DatabaseState* response) override;

  grpc::Status EpisodeUpdates(
      grpc::ServerContext* context, const podcaster::Empty* request,
      grpc::ServerWriter<podcaster::EpisodeUpdate>* response) override;

  grpc::Status Play(grpc::ServerContext* context,
                    const podcaster::EpisodeUri* request,
                    podcaster::Empty* response) override;

  grpc::Status Pause(grpc::ServerContext* context,
                     const podcaster::EpisodeUri* request,
                     podcaster::Empty* response) override;

  grpc::Status Resume(grpc::ServerContext* context,
                      const podcaster::EpisodeUri* request,
                      podcaster::Empty* response) override;

  grpc::Status Stop(grpc::ServerContext* context,
                    const podcaster::EpisodeUri* request,
                    podcaster::Empty* response) override;

  grpc::Status ShutdownIfNotPlaying(grpc::ServerContext* context,
                                    const podcaster::Empty* request,
                                    podcaster::Empty* response) override;

  void DeleteImpl(const podcaster::EpisodeUri& uri, QueueFlags flags);

  grpc::Status CleanupDownloads(grpc::ServerContext* context,
                                const podcaster::Empty* request,
                                podcaster::Empty* response) override;

  grpc::Status CleanupAll(grpc::ServerContext* context,
                          const podcaster::Empty* request,
                          podcaster::Empty* response) override;

  grpc::Status Delete(grpc::ServerContext* context,
                      const podcaster::EpisodeUri* request,
                      podcaster::Empty* response) override;

  grpc::Status Download(grpc::ServerContext* context,
                        const podcaster::EpisodeUri* request,
                        podcaster::Empty* response) override;

  grpc::Status CancelDownload(grpc::ServerContext* context,
                              const podcaster::EpisodeUri* request,
                              podcaster::Empty* response) override;

  grpc::Status GetConfigInfo(grpc::ServerContext* context,
                             const podcaster::Empty* request,
                             podcaster::ConfigInfo* response) override;

 private:
  void QueueUpdate(const podcaster::EpisodeUpdate& update, QueueFlags flags);

  void QueueDownloadStatus(const podcaster::EpisodeUri& request,
                           podcaster::DownloadStatus status,
                           QueueFlags flags = QueueFlags::kPersist);

  void QueueDownloadProgress(const podcaster::EpisodeUri& request,
                             float progress, float size,
                             QueueFlags flags = QueueFlags::kPersist);

  void QueuePlaybackStatus(const podcaster::EpisodeUri& request,
                           podcaster::PlaybackStatus status,
                           QueueFlags flags = QueueFlags::kPersist);

  void QueuePlaybackProgress(const podcaster::EpisodeUri& request,
                             int position_ms,
                             QueueFlags flags = QueueFlags::kPersist);

  void QueuePlaybackDuration(const podcaster::EpisodeUri& request,
                             int duration_ms,
                             QueueFlags flags = QueueFlags::kPersist);

  std::filesystem::path data_dir_;
  std::function<void()> shutdown_callback_;

  std::mutex download_mtx_;
  std::vector<ActiveDownload> downloads_in_progress_;

  std::mutex updates_mtx_;
  std::vector<podcaster::EpisodeUpdate> outbound_updates_;

  std::mutex db_mutex_;
  std::unique_ptr<podcaster::Database> db_;

  std::mutex playback_mtx_;
  PlaybackController playback_controller_;

  friend class PlaybackController;
  friend class XferInfoCallbackFunctor;
};
}  // namespace podcaster