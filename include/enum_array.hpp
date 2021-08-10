/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <array>
#include <utility>

// E is enumerator class and V is values type.
template<class E, class V,
         std::size_t size = static_cast<std::size_t>(E::_COUNT)>
// Wrapper around standard array, where key is a field of enumerator class.
class EnumArray {
  using arr_t = std::array<V, size>;

public:
  constexpr explicit EnumArray(arr_t other): m_arr(std::move(other)) {}
  [[nodiscard]] constexpr auto get(const E elem) const -> const V&
      { return m_arr.at(static_cast<std::size_t>(elem)); }

private:
  arr_t m_arr;
};
