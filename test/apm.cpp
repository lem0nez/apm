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
#include "tmp_file.hpp"

#include "internal/alt_stream.hpp"
#include "internal/args.hpp"
#include "internal/env.hpp"
#include "internal/tmp_dir.hpp"

using namespace std;

TEST_CASE("Program's arguments") {
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

TEST_CASE("Set a Java KeyStore") {
  constexpr auto OPTION{"--set-jks"};

  const TmpDir home_dir;
  Env::setup(home_dir.get_entry());

  AltStream
      alt_cout(cout),
      alt_cerr(cerr);
  error_condition err;
  Apm apm(err);

  {
    const TmpFile tmp_file;
    Args args{{}, OPTION, tmp_file.get_path()};
    AltStream alt_cin(cin);
    // Input: no alias name, a private key has a password.
    (*alt_cin).str("\nyes\n");

    CHECK(apm.run(args.get_argc(), args.get_argv()) == EXIT_SUCCESS);
  }
  CHECK((*alt_cerr).tellp() == streampos(0));

  constexpr array invalid_paths{"/", "non-existent-file"};
  for (const auto& p : invalid_paths) {
    Args args{{}, OPTION, p};
    (*alt_cerr).str({});

    CHECK(apm.run(args.get_argc(), args.get_argv()) == EXIT_FAILURE);
    CHECK((*alt_cerr).tellp() > streampos(0));
  }
}

TEST_CASE("Choose a theme") {
  const TmpDir home_dir;
  Env::setup(home_dir.get_entry());

  AltStream
      alt_cout(cout),
      alt_cerr(cerr);
  AltStream alt_cin(cin);

  constexpr array choices{"0\n", "1\n"};
  error_condition err;
  Apm apm(err);

  for (const auto& c : choices) {
    Args args{{}, "--choose-theme"};
    (*alt_cin).str(c);
    REQUIRE(apm.run(args.get_argc(), args.get_argv()) == EXIT_SUCCESS);
  }
  CHECK((*alt_cerr).tellp() == streampos(0));
}
