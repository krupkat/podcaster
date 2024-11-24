#pragma once

#include <functional>
#include <optional>

#include <format>

namespace utils {
template <typename TException, typename... TArgs>
void Throw(std::format_string<TArgs...> fmt, TArgs&&... args) {
  std::string error_msg = std::format(fmt, std::forward<TArgs>(args)...);
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
}  // namespace utils