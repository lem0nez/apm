/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <doctest/doctest.h>
#include "enum_array.hpp"

TEST_CASE("Get a value") {
  enum class Number{ONE, TWO, THREE, _COUNT};
  constexpr EnumArray<Number, unsigned short> numbers{1U, 2U, 3U};
  CHECK(numbers.get(Number::TWO) == 2U);
}
