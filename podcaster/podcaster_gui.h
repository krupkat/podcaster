// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <future>
#include <memory>

#include <grpcpp/create_channel.h>
#include <spdlog/spdlog.h>

#include "podcaster/action.h"
#include "podcaster/message.grpc.pb.h"
#include "podcaster/message.pb.h"
#include "podcaster/panels/about_window.h"
#include "podcaster/panels/cleanup_window.h"
#include "podcaster/panels/config_window.h"
#include "podcaster/panels/license_window.h"
#include "podcaster/panels/show_more_window.h"

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

  std::optional<ConfigInfo> GetConfigInfo() {
    grpc::ClientContext context;
    Empty request;
    ConfigInfo response;
    grpc::Status status = stub_->GetConfigInfo(&context, request, &response);
    if (!status.ok()) {
      spdlog::error("Fetching service config failed: {}",
                    status.error_message());
      return {};
    }
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

  void Cleanup(const CleanupExtra& extra) {
    grpc::ClientContext context;
    Empty request;
    Empty response;
    switch (extra.target) {
      case CleanupTarget::kDownloads:
        stub_->CleanupDownloads(&context, request, &response);
        break;
      case CleanupTarget::kAll:
        stub_->CleanupAll(&context, request, &response);
        break;
    }
  }

 private:
  std::unique_ptr<podcaster::Podcaster::Stub> stub_;
};

enum class ServiceStatus { kOnline, kOffline };

class PodcasterGui {
 public:
  PodcasterGui(PodcasterClient client, std::filesystem::path exe_path)
      : client_(std::move(client)),
        license_window_(exe_path),
        about_window_(exe_path) {
    state_ = client_.GetState();
    auto config_info = client_.GetConfigInfo();
    if (not config_info or (config_info->config().feed_size() == 0)) {
      config_window_.Open(config_info);
    }
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
  LicenseWindow license_window_;
  AboutWindow about_window_;
  ConfigWindow config_window_;
  CleanupWindow cleanup_window_;

  // futures
  std::future<DatabaseState> refresh_future_;

  // extra gui stuff
  int selected_tab_ = 0;
  bool last_top_row_in_focus_ = true;
  ServiceStatus service_status_ = ServiceStatus::kOnline;
};

}  // namespace podcaster