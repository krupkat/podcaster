// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include "podcaster/message.pb.h"
#include "podcaster/panels/simple_window.h"

namespace podcaster {

class ConfigWindow : public SimpleWindow<std::optional<ConfigInfo>> {
  const char* Title() const override { return "Config"; }
  Action DrawImpl(const Action& incoming_action) override;
  void OpenImpl(const std::optional<ConfigInfo>& config) override;

  std::optional<ConfigInfo> config_;
};
}  // namespace podcaster