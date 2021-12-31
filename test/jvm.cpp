/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <string>
#include <doctest/doctest.h>
#include "internal/env.hpp"

TEST_CASE("JVM tools") {
  const auto jvm{Env::get_jvm()};
  std::string out, err;

  // Java compiler.
  CHECK_NOTHROW(jvm->javac({}, out, err));
  CHECK(jvm->javac({"-help"}, out, err) == 0);
  CHECK(jvm->javac({"this-file-does-not-exist"}, out, err) != 0);
  CHECK_FALSE(err.empty());

  // d8.
  CHECK(jvm->d8({"--help"}, out, err) == 0);
  CHECK_FALSE(out.empty());
  CHECK(err.empty());
  CHECK(jvm->d8({"--unknown-option-1", "--unknown-option-2"}, out, err) != 0);
  CHECK_FALSE(err.empty());

  // apksigner.
  CHECK(jvm->apksigner({"--version"}, out, err) == 0);
  CHECK_FALSE(out.empty());
  CHECK(err.empty());
  CHECK(jvm->apksigner({"--unknown-option"}, out, err) != 0);
  CHECK_FALSE(err.empty());
}
