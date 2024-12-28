// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "podcaster/panels/simple_window.h"

namespace podcaster {

class CleanupWindow
    : public SimpleWindow<> {
  const char* Title() const override { return "Cleanup"; }
  Action DrawImpl(const Action& incoming_action) override;
};
}  // namespace podcaster