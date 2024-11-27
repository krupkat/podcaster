#pragma once

#include <optional>
#include <string_view>

#include <capnp/rpc-twoparty.h>

#include "schema.capnp.h"

namespace capnproto {

struct ClientCtx {
  kj::Own<capnp::TwoPartyClient> client;
  podcaster::PodcasterService::Client podcaster_service;
};

std::optional<ClientCtx> Connect(std::string_view address, kj::AsyncIoContext& async_io);

}  // namespace capnproto