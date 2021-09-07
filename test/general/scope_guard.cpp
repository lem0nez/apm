/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <doctest/doctest.h>
#include "general/scope_guard.hpp"

TEST_CASE("Scope guard") {
  bool flag{};
  const auto toggle{[&flag] { flag = !flag; }};

  // No handler.
  static_cast<void>(ScopeGuard());
  // Pass a handler to constructor.
  static_cast<void>(ScopeGuard(toggle));
  REQUIRE(flag);

  // Call the assignment operator.
  ScopeGuard() = toggle;
  CHECK_FALSE(flag);
}
