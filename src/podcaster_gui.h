#pragma once

#include <memory>
#include <variant>

#include <grpcpp/create_channel.h>
#include <spdlog/spdlog.h>

#include "message.grpc.pb.h"

namespace podcaster {

enum class ActionType { kIdle, kRefresh, kDownloadEpisode, kPlayEpisode };

struct EpisodeExtra {
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

  DatabaseState GetState() {
    grpc::ClientContext context;
    Empty request;
    DatabaseState response;
    stub_->State(&context, request, &response);
    return response;
  }

  void Refresh() {
    grpc::ClientContext context;
    Empty request;
    std::unique_ptr<grpc::ClientReader<RefreshProgress>> reader(
        stub_->Refresh(&context, request));

    RefreshProgress progress;
    while (reader->Read(&progress)) {
      spdlog::info("Refreshed {}/{}", progress.refreshed_feeds(),
                   progress.total_feeds());
    }

    grpc::Status status = reader->Finish();
    if (!status.ok()) {
      spdlog::error("Refresh failed: {}", status.error_message());
    } else {
      spdlog::info("Refresh completed");
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

  void Run();

 private:
  Action Draw();

  PodcasterClient client_;
  DatabaseState state_;
};

}  // namespace podcaster