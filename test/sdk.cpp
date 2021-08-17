/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <system_error>

#include <doctest/doctest.h>

#include "apm.hpp"
#include "enum_array.hpp"
#include "sdk.hpp"

#include "internal/alt_stream.hpp"
#include "internal/args.hpp"
#include "internal/env.hpp"
#include "internal/tmp_dir.hpp"

using namespace std;

TEST_CASE("Install SDK") {
  using namespace filesystem;

  const TmpDir home_dir;
  Env::setup(home_dir.get_entry());
  error_condition err;
  Args args{{}, "-s"};

  MESSAGE("SDK installation in progress... So as not run this test case every "
          "time, you can exclude it via -tce (--test-case-exclude) option");
  // Using separate namespace to restore standard
  // streams after the installation is complete.
  {
    AltStream
        alt_cout(cout),
        alt_cerr(cerr);
    Apm apm(err);

    REQUIRE(apm.run(args.get_argc(), args.get_argv()) == EXIT_SUCCESS);
    CHECK((*alt_cerr).tellp() == streampos(0));
  }

  Sdk sdk;
  for (size_t t{}; t != size<Sdk::Tool>(); ++t) {
    CHECK(is_regular_file(sdk.get_tool_path(static_cast<Sdk::Tool>(t))));
  }
  for (size_t j{}; j != size<Sdk::Jar>(); ++j) {
    CHECK(is_regular_file(sdk.get_jar_path(static_cast<Sdk::Jar>(j))));
  }
  for (size_t f{}; f != size<Sdk::File>(); ++f) {
    CHECK(is_regular_file(sdk.get_file_path(static_cast<Sdk::File>(f))));
  }
}
