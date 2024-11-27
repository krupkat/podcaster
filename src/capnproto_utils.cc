#include "capnproto_utils.h"

#include <spdlog/spdlog.h>

namespace capnproto {

std::optional<ClientCtx> Connect(std::string_view address, kj::AsyncIoContext& async_io) {
  kj::StringPtr address_str{address.data(), address.size()};

  kj::Own<capnp::TwoPartyClient> client;
  podcaster::PodcasterService::Client podcaster_service{nullptr};

  try {
    kj::Network& network = async_io.provider->getNetwork();
    kj::Own<kj::NetworkAddress> addr = network.parseAddress(address_str).wait(async_io.waitScope);
    kj::Own<kj::AsyncIoStream> conn = addr->connect().wait(async_io.waitScope);

    client = kj::heap<capnp::TwoPartyClient>(*conn).attach(kj::mv(conn));
    podcaster_service = client->bootstrap().castAs<podcaster::PodcasterService>();
  } catch (kj::Exception& e) {
    spdlog::error("Error connecting to podcaster service: {}", e.getDescription().cStr());
    return {};
  }

  return ClientCtx{kj::mv(client), podcaster_service};
}

}  // namespace capnproto