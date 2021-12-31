/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <system_error>

#include <doctest/doctest.h>
#include "apm.hpp"
#include "project.hpp"

#include "internal/alt_stream.hpp"
#include "internal/args.hpp"
#include "internal/env.hpp"
#include "internal/tmp_dir.hpp"

using namespace std;

TEST_CASE("Create projects") {
  // Project properties: application name, package name,
  // minimum API level and whether to add .gitignore file.
  constexpr array props{
    " \na.a\n21\n\n",
    "App \u03b1\nA0.z9.Z_\n24\nN\n",
    "Cute name\ncom.example.cuteapp\n26\nY\n"
  };

  const TmpDir projects_dir;
  const auto projects_path{projects_dir.get_entry().path()};
  unsigned short project_counter{};

  Env::setup(Env::get_sdk_home());
  error_condition err;
  Apm apm(err);

  AltStream
      alt_cout(cout),
      alt_cerr(cerr);
  AltStream alt_cin(cin);

  for (const auto& p : props) {
    Args args{{}, "--create", projects_path / to_string(++project_counter)};
    (*alt_cin).str(p);

    REQUIRE(apm.run(args.get_argc(), args.get_argv()) == EXIT_SUCCESS);
    REQUIRE((*alt_cerr).tellp() == streampos(0));
  }
}
