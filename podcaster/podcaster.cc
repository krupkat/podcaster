// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <memory>
#include <signal.h>  // NOLINT(modernize-deprecated-headers)

#include <curlpp/cURLpp.hpp>
#include <grpcpp/grpcpp.h>
#include <SDL_mixer.h>

#include "podcaster/podcaster_impl.h"
#include "podcaster/sdl_mixer_utils.h"

std::atomic<int> cancel_service{0};

void CancelHandler(int /*signal*/) {
  cancel_service.fetch_add(1);
  cancel_service.notify_one();
}

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
  podcaster::PodcasterImpl service(data_dir, [&] { CancelHandler(0); });

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  spdlog::info("Server listening on {}", server_address);

  cancel_service.wait(0);
  server->Shutdown();
}