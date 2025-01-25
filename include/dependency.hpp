#pragma once

#include <memory>


template<typename... T>
class Dependency
{
public:
    Dependency(std::shared_ptr<T>... dependencies):
        _dependencies(dependencies...)
    {}
    virtual ~Dependency() = default;

protected:
    std::tuple<std::shared_ptr<T>...> _dependencies;

protected:
  //template<typename U, typename... V>
  //constexpr bool contains()
  //{
  //    return (std::is_same_v<U, V> || ...);
  //}

    template<typename U>
    std::shared_ptr<U> get()
    {
        // static_assert(contains<U, T...>(), "Trying to access a non existent dependency");
        return std::get<std::shared_ptr<U>>(_dependencies);
    }
};

