#include <capnp/rpc-twoparty.h>
#include <curlpp/cURLpp.hpp>
#include <kj/async-io.h>

#include "schema.capnp.h"

template <typename TProgressType>
class SessionImpl : public podcaster::Session<TProgressType>::Server {
 public:
  using Base = podcaster::Session<TProgressType>::Server;

  kj::Promise<void> pause(Base::PauseContext context) override { return kj::READY_NOW; }
  kj::Promise<void> resume(Base::ResumeContext context) override { return kj::READY_NOW; }
  kj::Promise<void> close(Base::CloseContext context) override { return kj::READY_NOW; }
  kj::Promise<void> progress(Base::ProgressContext context) override {
    auto results = context.getResults();
    if constexpr (std::is_same_v<TProgressType, podcaster::PlaybackProgress>) {
      auto progress = results.initProgress().initInProgress();
      progress.setElapsedSeconds(10);
      progress.setTotalSeconds(100);
    } else {
      auto progress = results.initProgress().initInProgress();
      progress.setDownloadedBytes(10);
      progress.setTotalBytes(100);
    }
    return kj::READY_NOW;
  }
};

class EpisodeImpl : public podcaster::Episode::Server {
 public:
  kj::Promise<void> play(PlayContext context) override {
    auto results = context.getResults();
    results.setProgress(kj::heap<SessionImpl<podcaster::PlaybackProgress>>());
    return kj::READY_NOW;
  }
  kj::Promise<void> download(DownloadContext context) override {
    auto results = context.getResults();
    results.setProgress(kj::heap<SessionImpl<podcaster::DownloadProgress>>());
    return kj::READY_NOW;
  }
};

class PodcasterServiceImpl : public podcaster::PodcasterService::Server {
 public:
  kj::Promise<void> refresh(RefreshContext context) override {
    auto results = context.getResults();
    auto podcasts = results.initPodcasts(1);
    podcasts[0].setTitle("Hello, World!");
    auto episodes = podcasts[0].initEpisodes(1);
    episodes[0].setTitle("Episode 1");
    episodes[0].setDescription("This is the first episode.");
    return kj::READY_NOW;
  }

  kj::Promise<void> connect(ConnectContext context) override {
    auto results = context.getResults();
    results.setEpisode(kj::heap<EpisodeImpl>());
    return kj::READY_NOW;
  }
};

int main() {
  curlpp::Cleanup cleanup;

  auto async_io = kj::setupAsyncIo();

  kj::Network& network = async_io.provider->getNetwork();
  kj::Own<kj::NetworkAddress> addr =
      network.parseAddress("unix-abstract:podcaster").wait(async_io.waitScope);
  kj::Own<kj::ConnectionReceiver> listener = addr->listen();

  capnp::TwoPartyServer server(kj::heap<PodcasterServiceImpl>());

  server.listen(*listener).wait(async_io.waitScope);
}