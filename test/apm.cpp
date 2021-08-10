/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <array>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <doctest/doctest.h>

#include "apm.hpp"
#include "internal/args.hpp"
#include "internal/buf_restorer.hpp"
#include "internal/env.hpp"
#include "internal/tmp_dir.hpp"

using namespace std;

TEST_CASE("Pass arguments") {
  TmpDir tmp_dir;
  Env::setup(tmp_dir.get_entry());

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

  BufRestorer
      cout_restorer(cout),
      cerr_restorer(cerr);
  ostringstream cout_oss, cerr_oss;
  cout.rdbuf(cout_oss.rdbuf());
  cerr.rdbuf(cerr_oss.rdbuf());

  for (auto& a : valid_args) {
    Apm apm;
    REQUIRE(apm.run(a.get_argc(), a.get_argv()) == EXIT_SUCCESS);
    REQUIRE(cerr_oss.tellp() == streampos(0));
  }

  for (auto& a : invalid_args) {
    Apm apm;
    REQUIRE(apm.run(a.get_argc(), a.get_argv()) == EXIT_FAILURE);
    REQUIRE(cerr_oss.tellp() > streamsize(0));
    cerr_oss.str({});
  }
}
