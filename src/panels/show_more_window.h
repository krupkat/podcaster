#pragma once

#include "action.h"
#include "panels/simple_window.h"

namespace podcaster {
class ShowMoreWindow : public SimpleWindow<ShowMoreExtra, true> {
  const char* Title() const override { return "Episode details"; }

  void OpenImpl(const ShowMoreExtra& extra) override {
    title_ = extra.episode_title;
    if (extra.episode_description_long.size() > 0 &&
        extra.episode_description_short.size() >= 3) {
      auto ellipsis_idx = extra.episode_description_short.size() - 3;
      description_ = extra.episode_description_short.substr(0, ellipsis_idx) +
                     extra.episode_description_long;
    } else {
      description_ = extra.episode_description_short;
    }
  }

  Action DrawImpl(const Action& incoming_action) override {
    Action action = {};

    ImGui::Text("%s", title_.c_str());
    ImGui::Separator();
    ImGui::TextWrapped("%s", description_.c_str());

    return action;
  }

  std::string title_;
  std::string description_;
};
}  // namespace podcaster