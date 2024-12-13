#pragma once

#include <future>
#include <memory>
#include <variant>

#include <grpcpp/create_channel.h>
#include <spdlog/spdlog.h>

#include "message.grpc.pb.h"
#include "message.pb.h"

namespace podcaster {

enum class ActionType {
  kIdle,
  kRefresh,
  kDownloadEpisode,
  kPlayEpisode,
  kPauseEpisode,
  kResumeEpisode,
  kStopEpisode,
  kDeleteEpisode,
  kCancelDownload,
  kFlipPanes
};

struct EpisodeExtra {
  std::string podcast_uri;
  std::string episode_uri;
};

struct Action {
  ActionType type = ActionType::kIdle;
  std::variant<std::monostate, EpisodeExtra> extra;
};

inline Action& operator|=(Action& lhs, const Action& rhs) {
  if (rhs.type != ActionType::kIdle) {
    lhs = rhs;
  }
  return lhs;
}

class PodcasterClient {
 public:
  PodcasterClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(Podcaster::NewStub(channel)) {}

  ~PodcasterClient() {
    if (stub_) {
      Shutdown();
    }
  }

  PodcasterClient(const PodcasterClient& other) = delete;
  PodcasterClient& operator=(const PodcasterClient& other) = delete;
  PodcasterClient(PodcasterClient&& other) = default;
  PodcasterClient& operator=(PodcasterClient&& other) = default;

  void Shutdown() {
    grpc::ClientContext context;
    Empty request;
    Empty response;
    stub_->ShutdownIfNotPlaying(&context, request, &response);
  }

  DatabaseState GetState() {
    grpc::ClientContext context;
    Empty request;
    DatabaseState response;
    stub_->State(&context, request, &response);
    return response;
  }

  DatabaseState Refresh() {
    grpc::ClientContext context;
    Empty request;
    DatabaseState response;
    stub_->Refresh(&context, request, &response);
    return response;
  }

  std::vector<EpisodeUpdate> ReadUpdates() {
    grpc::ClientContext context;
    Empty request;
    std::unique_ptr<grpc::ClientReader<EpisodeUpdate>> reader(
        stub_->EpisodeUpdates(&context, request));

    std::vector<EpisodeUpdate> updates;

    EpisodeUpdate update;
    while (reader->Read(&update)) {
      updates.push_back(update);
    }

    grpc::Status status = reader->Finish();
    if (!status.ok()) {
      spdlog::error("Read updates failed: {}", status.error_message());
    }
    return updates;
  }

  void EpisodeAction(const std::string& podcast_uri,
                     const std::string& episode_uri, ActionType action) {
    grpc::ClientContext context;
    Empty response;
    EpisodeUri uri;
    uri.set_podcast_uri(podcast_uri);
    uri.set_episode_uri(episode_uri);
    switch (action) {
      case ActionType::kDownloadEpisode:
        stub_->Download(&context, uri, &response);
        break;
      case ActionType::kCancelDownload:
        stub_->CancelDownload(&context, uri, &response);
        break;
      case ActionType::kPlayEpisode:
        stub_->Play(&context, uri, &response);
        break;
      case ActionType::kPauseEpisode:
        stub_->Pause(&context, uri, &response);
        break;
      case ActionType::kResumeEpisode:
        stub_->Resume(&context, uri, &response);
        break;
      case ActionType::kStopEpisode:
        stub_->Stop(&context, uri, &response);
        break;
      case ActionType::kDeleteEpisode:
        stub_->Delete(&context, uri, &response);
        break;
      default:
        break;
    }
  }

 private:
  std::unique_ptr<podcaster::Podcaster::Stub> stub_;
};

class PodcasterGui {
 public:
  PodcasterGui(PodcasterClient client) : client_(std::move(client)) {
    state_ = client_.GetState();
  }

  void Run(const Action& incoming_action);

 private:
  Action Draw(const Action& incoming_action);

  PodcasterClient client_;
  DatabaseState state_;

  std::future<DatabaseState> refresh_future_;
  int selected_tab_ = 0;
};

}  // namespace podcaster