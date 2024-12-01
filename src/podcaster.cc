#include <filesystem>
#include <thread>

#include <curlpp/cURLpp.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include "database.h"
#include "message.grpc.pb.h"
#include "message.pb.h"

class PodcasterImpl final : public podcaster::Podcaster::Service {
 public:
  explicit PodcasterImpl(std::filesystem::path data_dir)
      : db_(std::make_unique<podcaster::Database>(data_dir)) {}

  grpc::Status State(grpc::ServerContext* context,
                     const podcaster::Empty* request,
                     podcaster::DatabaseState* response) override {
    auto state = db_->GetState();
    response->CopyFrom(state);
    return grpc::Status::OK;
  }

  grpc::Status Refresh(
      grpc::ServerContext* context, const podcaster::Empty* request,
      grpc::ServerWriter<podcaster::RefreshProgress>* response) override {
    for (int i = 0; i < 10; i++) {
      podcaster::RefreshProgress progress;
      progress.set_refreshed_feeds(i + 1);
      progress.set_total_feeds(10);
      response->Write(progress);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    podcaster::Podcast pod1;
    pod1.set_podcast_uri("https://example.com/podcast1");
    pod1.set_title("Podcast 1");
    auto* ep1 = pod1.add_episodes();
    ep1->set_episode_uri("https://example.com/podcast1/episode1");
    ep1->set_title("Episode 1");
    ep1->set_description("Description 1");
    auto* ep2 = pod1.add_episodes();
    ep2->set_episode_uri("https://example.com/podcast1/episode2");
    ep2->set_title("Episode 2");
    ep2->set_description("Description 2");
    db_->SavePodcast(pod1);

    podcaster::Podcast pod2;
    pod2.set_podcast_uri("https://example.com/podcast2");
    pod2.set_title("Podcast 2");
    auto* ep3 = pod2.add_episodes();
    ep3->set_episode_uri("https://example.com/podcast2/episode1");
    ep3->set_title("Episode 1");
    ep3->set_description("Description 1");
    db_->SavePodcast(pod2);
    db_->SaveState();

    return grpc::Status::OK;
  }

 private:
  std::unique_ptr<podcaster::Database> db_;
};

int main(int argc, char** argv) {
  curlpp::Cleanup cleanup;

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
  server->Wait();
}