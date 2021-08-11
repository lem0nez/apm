/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <array>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <system_error>

#include <doctest/doctest.h>

#include "apm.hpp"
#include "internal/alt_stream.hpp"
#include "internal/args.hpp"
#include "internal/env.hpp"
#include "internal/tmp_dir.hpp"

using namespace std;

TEST_CASE("Pass arguments") {
  TmpDir home_dir;
  Env::setup(home_dir.get_entry());

  array<Args, 6U> valid_args{{
    {{}},
    {"apm", "--colors=0"},
    {"./apm", "--colors", "8"},
    {"../apm", "--colors=256"},
    {"/usr/local/bin/apm", "-h"},
    {"/usr/bin/apm", "--version"}
  }};

  array<Args, 3U> invalid_args{{
    {{}, "--unknown-option"},
    {{}, "--colors=auto"},
    {{}, "--help="}
  }};

  AltStream
      alt_cout(cout),
      alt_cerr(cerr);

  error_condition err;
  Apm apm(err);
  REQUIRE_FALSE(err);

  for (auto& a : valid_args) {
    REQUIRE(apm.run(a.get_argc(), a.get_argv()) == EXIT_SUCCESS);
    REQUIRE((*alt_cerr).tellp() == streampos(0));
  }

  for (auto& a : invalid_args) {
    REQUIRE(apm.run(a.get_argc(), a.get_argv()) == EXIT_FAILURE);
    REQUIRE((*alt_cerr).tellp() > streampos(0));
    (*alt_cerr).str({});
  }
}

TEST_CASE("Choose theme") {
  const TmpDir home_dir;
  Env::setup(home_dir.get_entry());

  istringstream alt_cin;
  AltStream
      alt_cout(cout),
      alt_cerr(cerr);
  error_condition err;
  Apm apm(err);

  alt_cin.str("0\n");
  CHECK_NOTHROW(apm.request_theme(alt_cin));
  alt_cin.str("1\n");
  CHECK_NOTHROW(apm.request_theme(alt_cin));
}
