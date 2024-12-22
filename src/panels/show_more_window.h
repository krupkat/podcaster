#pragma once

#include "action.h"
#include "panels/simple_window.h"

namespace podcaster {
class ShowMoreWindow
    : public SimpleWindow<ShowMoreExtra, WindowTraits::kScrollEvents> {
  const char* Title() const override { return "Episode details"; }
  void OpenImpl(const ShowMoreExtra& extra) override;
  Action DrawImpl(const Action& incoming_action) override;

  std::string title_;
  std::string description_;
};
}  // namespace podcaster