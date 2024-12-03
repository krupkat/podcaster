#include <filesystem>
#include <pugixml.hpp>
#include <thread>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>
#include <spdlog/spdlog.h>

#include "database.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <google/protobuf/text_format.h>

podcaster::Podcast DonwloadAndParseFeed(
    const std::string& feed_uri, const std::filesystem::path& cache_dir) {
  std::stringstream feed;

  curlpp::Easy my_request;
  my_request.setOpt<curlpp::options::Url>(feed_uri);
  my_request.setOpt<curlpp::options::WriteStream>(&feed);
  my_request.perform();

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
        db_(std::make_unique<podcaster::Database>(data_dir)) {}

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
    auto config_path = data_dir_ / "config.textproto";

    podcaster::Config config;

    if (std::filesystem::exists(config_path)) {
      std::ifstream config_file(config_path);
      if (config_file.is_open()) {
        google::protobuf::io::IstreamInputStream input_stream(&config_file);
        google::protobuf::TextFormat::Parse(&input_stream, &config);
      }
    }

    int i = 1;
    for (const auto& feed : config.feed()) {
      auto podcast = DonwloadAndParseFeed(feed, data_dir_);
      db_->SavePodcast(podcast);

      podcaster::RefreshProgress progress;
      progress.set_refreshed_feeds(i);
      progress.set_total_feeds(config.feed_size());
      response->Write(progress);
    }

    db_->SaveState();

    return grpc::Status::OK;
  }

 private:
  std::filesystem::path data_dir_;
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