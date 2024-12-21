#pragma once

#include <tuple>
#include <type_traits>

#include <imgui.h>

#include "action.h"

namespace podcaster {

struct EmptyState {};

template <typename TState = EmptyState, bool scroll_events = false,
          typename SubwindowList = std::tuple<>>
class SimpleWindow {
  static constexpr float kPadding = 10.0f;

 public:
  template <typename State = TState>
  void Open(const State& state)
    requires(not std::is_same_v<TState, EmptyState>)
  {
    open_ = true;
    scroll_up_ = true;
    OpenImpl(state);
  }

  void Open()
    requires std::is_same_v<TState, EmptyState>
  {
    open_ = true;
    scroll_up_ = true;
  }

  bool IsOpen() const { return open_; }

  template <typename TWindowType, typename... Args>
  void OpenSubwindow(Args&&... args) {
    std::get<TWindowType>(subwindows_).Open(std::forward<Args>(args)...);
  }

  Action Draw(const Action& incoming_action) {
    Action action = {};

    bool any_subwindow_open = std::apply(
        [&](auto&&... subwindow) { return (subwindow.IsOpen() || ...); },
        subwindows_);

    if (open_ and not any_subwindow_open) {
      const ImGuiViewport* viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(
          ImVec2(viewport->Pos.x + kPadding, viewport->Pos.y + kPadding),
          ImGuiCond_Always);
      ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - kPadding * 2,
                                      viewport->Size.y - kPadding * 2),
                               ImGuiCond_Always);

      if (scroll_up_) {
        ImGui::SetNextWindowScroll(ImVec2(-1.0f, 0.0f));
        scroll_up_ = false;
      }

      if (ImGui::Begin(Title(), nullptr,
                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoSavedSettings)) {
        DrawImpl(incoming_action);

        if (ImGui::Button("Close")) {
          open_ = false;
        }
        if (incoming_action.type == ActionType::kBack) {
          open_ = false;
        }
        if constexpr (scroll_events) {
          if (incoming_action.type == ActionType::kScrollUp) {
            ImGui::SetScrollY(ImGui::GetScrollY() - 100);
          }
          if (incoming_action.type == ActionType::kScrollDown) {
            ImGui::SetScrollY(ImGui::GetScrollY() + 100);
          }
        }
      }
      ImGui::End();
    }

    return std::apply(
        [&](auto&&... subwindow) {
          return (action | ... | subwindow.Draw(incoming_action));
        },
        subwindows_);
  }

 private:
  virtual const char* Title() const = 0;
  virtual Action DrawImpl(const Action& incoming_action) = 0;

  virtual void OpenImpl(const TState& state) {};

  bool open_ = false;
  bool scroll_up_ = false;

  SubwindowList subwindows_ = {};
};

}  // namespace podcaster