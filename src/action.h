#pragma once

#include <string>
#include <variant>

namespace podcaster {
enum class ActionType {
  kIdle,
  kRefresh,
  kDownloadEpisode,
  kPlayEpisode,
  kPauseEpisode,
  kResumeEpisode,
  kStopEpisode,
  kDeleteEpisode,
  kCancelDownload,
  kShowMore,
  kShowAbout,
  kFlipPanes,
  kScrollUp,
  kScrollDown,
  kBack
};

struct EpisodeExtra {
  std::string podcast_uri;
  std::string episode_uri;
};

struct ShowMoreExtra {
  std::string episode_title;
  std::string episode_description_short;
  std::string episode_description_long;
};

struct Action {
  ActionType type = ActionType::kIdle;
  std::variant<std::monostate, EpisodeExtra, ShowMoreExtra> extra;
};

inline Action& operator|=(Action& lhs, const Action& rhs) {
  if (rhs.type != ActionType::kIdle) {
    lhs = rhs;
  }
  return lhs;
}

inline Action operator|(const Action& lhs, const Action& rhs) {
  Action result = lhs;
  return result |= rhs;
}

}  // namespace podcaster