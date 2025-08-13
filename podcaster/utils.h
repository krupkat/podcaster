// SPDX-FileCopyrightText: 2024 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <future>
#include <optional>

#include <spdlog/fmt/fmt.h>

namespace utils {
template <typename TException, typename... TArgs>
void Throw(fmt::format_string<TArgs...> fmt, TArgs&&... args) {
  std::string error_msg = fmt::format(fmt, std::forward<TArgs>(args)...);
  throw TException(error_msg);
}

class DestructorCallback {
 public:
  template <typename TFunc>
  explicit DestructorCallback(TFunc func) : callback_(func) {}
  ~DestructorCallback() {
    if (callback_) {
      (*callback_)();
    }
  };

  DestructorCallback(const DestructorCallback& other) = delete;
  DestructorCallback& operator=(const DestructorCallback&) = delete;
  DestructorCallback(DestructorCallback&& other) { *this = std::move(other); };
  DestructorCallback& operator=(DestructorCallback&& other) {
    if (this != &other) {
      std::swap(this->callback_, other.callback_);
    }
    return *this;
  };

 private:
  std::optional<std::function<void()>> callback_;
};

template <typename TType>
bool IsReady(const std::future<TType>& future) {
  return future.valid() &&
         future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

}  // namespace utils