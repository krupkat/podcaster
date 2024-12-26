#pragma once

#include <optional>

#include "podcaster/message.pb.h"
#include "podcaster/panels/simple_window.h"

namespace podcaster {

class ConfigWindow : public SimpleWindow<std::optional<ConfigDetails>,
                                         WindowTraits::kScrollEvents> {
  const char* Title() const override { return "Config"; }
  Action DrawImpl(const Action& incoming_action) override;
  void OpenImpl(const std::optional<ConfigDetails>& config) override;

  std::optional<ConfigDetails> config_;
};
}  // namespace podcaster