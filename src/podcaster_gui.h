#pragma once

#include <future>
#include <memory>
#include <variant>

#include <grpcpp/create_channel.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "action.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include "simple_window.h"

namespace podcaster {

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

  std::optional<std::vector<EpisodeUpdate>> ReadUpdates() {
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
      return {};
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

enum class ServiceStatus { kOnline, kOffline };

class ShowMoreWindow : public SimpleWindow<ShowMoreExtra> {
  const char* Title() const override { return "Episode details"; }

  void OpenImpl(const ShowMoreExtra& extra) override {
    title_ = extra.episode_title;
    if (extra.episode_description_long.size() > 0 &&
        extra.episode_description_short.size() >= 3) {
      auto ellipsis_idx = extra.episode_description_short.size() - 3;
      description_ = extra.episode_description_short.substr(0, ellipsis_idx) +
                     extra.episode_description_long;
    } else {
      description_ = extra.episode_description_short;
    }
  }

  Action DrawImpl(const Action& incoming_action) override {
    Action action = {};

    ImGui::Text("%s", title_.c_str());
    ImGui::Separator();
    ImGui::TextWrapped("%s", description_.c_str());

    return action;
  }

  std::string title_;
  std::string description_;
};

class PodcasterGui {
 public:
  PodcasterGui(PodcasterClient client) : client_(std::move(client)) {
    state_ = client_.GetState();
  }

  void Run(const Action& incoming_action);

  void UpdateServiceStatus(ServiceStatus status);

 private:
  Action Draw(const Action& incoming_action);

  // db stuff
  PodcasterClient client_;
  DatabaseState state_;

  // subwindows
  ShowMoreWindow show_more_window_;

  // futures
  std::future<DatabaseState> refresh_future_;

  // extra gui stuff
  int selected_tab_ = 0;
  bool last_top_row_in_focus_ = true;
  ServiceStatus service_status_ = ServiceStatus::kOnline;
};

}  // namespace podcaster