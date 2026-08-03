#pragma once

#include <utility>
#include <variant>

namespace beez
{

template <typename T, typename E> class Expected
{
  public:
    Expected(T value) : data_(std::move(value)) {}
    Expected(E error) : data_(std::move(error)) {}

    [[nodiscard]] bool hasValue() const
    {
        return std::holds_alternative<T>(data_);
    }

    [[nodiscard]] explicit operator bool() const
    {
        return hasValue();
    }

    [[nodiscard]] T& value()
    {
        return std::get<T>(data_);
    }

    [[nodiscard]] const T& value() const
    {
        return std::get<T>(data_);
    }

    [[nodiscard]] E& error()
    {
        return std::get<E>(data_);
    }

    [[nodiscard]] const E& error() const
    {
        return std::get<E>(data_);
    }

  private:
    std::variant<T, E> data_;
};

template <typename E> class Expected<void, E>
{
  public:
    Expected() = default;
    Expected(E error) : error_(std::move(error)), hasValue_(false) {}

    [[nodiscard]] bool hasValue() const
    {
        return hasValue_;
    }

    [[nodiscard]] explicit operator bool() const
    {
        return hasValue_;
    }

    [[nodiscard]] E& error()
    {
        return error_;
    }

    [[nodiscard]] const E& error() const
    {
        return error_;
    }

  private:
    E error_ {};
    bool hasValue_ = true;
};

}  // namespace beez
