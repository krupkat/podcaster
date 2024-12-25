#include "podcaster/panels/show_more_window.h"

namespace podcaster {

void ShowMoreWindow::OpenImpl(const ShowMoreExtra& extra) {
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

Action ShowMoreWindow::DrawImpl(const Action& incoming_action) {
  Action action = {};

  ImGui::Text("%s", title_.c_str());
  ImGui::Separator();
  ImGui::TextWrapped("%s", description_.c_str());

  return action;
}

}  // namespace podcaster