/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <array>
#include <type_traits>
#include <utility>

// Returns number of enumerator fields.
template<typename T> constexpr auto size() {
  static_assert(std::is_enum_v<T>, "T must be enumeration");
  return static_cast<std::size_t>(T::_COUNT);
}

// E is enumerator class and V is values type.
template<typename E, typename V, std::size_t size = size<E>()>
// Wrapper around standard array, where key is a field of enumerator class.
class EnumArray {
  using arr_t = std::array<V, size>;

public:
  template<typename... Vals> constexpr explicit EnumArray(Vals&&... vals):
      m_arr{std::forward<V>(vals)...} {}
  [[nodiscard]] constexpr auto get(const E elem) const -> const V&
      { return m_arr.at(static_cast<std::size_t>(elem)); }

private:
  arr_t m_arr;
};
