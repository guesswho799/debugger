#pragma once

#include <memory>

template <typename... T> class Dependency {
public:
  Dependency(std::shared_ptr<T>... dependencies)
      : _dependencies(std::move(dependencies)...) {}
  virtual ~Dependency() = default;

protected:
  template <typename U, typename... V> static consteval bool contains() {
    return (std::is_same_v<U, V> || ...);
  }

  template <typename U> std::shared_ptr<U> &get() noexcept {
    static_assert(contains<U, T...>(),
                  "Trying to access a non existent dependency");
    return std::get<std::shared_ptr<U>>(_dependencies);
  }

protected:
  std::tuple<std::shared_ptr<T>...> _dependencies;
};
