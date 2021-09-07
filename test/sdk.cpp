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
#include "internal/alt_stream.hpp"
#include "internal/args.hpp"
#include "internal/env.hpp"
#include "internal/tmp_dir.hpp"

#include "general/enum_array.hpp"
#include "apm.hpp"
#include "sdk.hpp"

using namespace std;

TEST_CASE("Install SDKs") {
  MESSAGE("SDKs installation in progress... So as not run this test case every "
          "time, you can exclude it via -tce (--test-case-exclude) option");

  constexpr array sdk_apis{28U};
  error_condition err;

  for (const auto a : sdk_apis) {
    const TmpDir home_dir;
    Env::setup(home_dir.get_entry());
    Args args{{}, "-s"};

    // Using separate namespace to restore standard
    // streams after the installation is complete.
    {
      AltStream
          alt_cout(cout),
          alt_cerr(cerr);
      AltStream alt_cin(cin);
      (*alt_cin).str(to_string(a) + '\n');

      Apm apm(err);
      REQUIRE(apm.run(args.get_argc(), args.get_argv()) == EXIT_SUCCESS);
      CHECK((*alt_cerr).tellp() == streampos(0));
    }

    const Sdk sdk;
    // Check if all files are installed.
    for (size_t t{}; t != size<Sdk::Tool>(); ++t) {
      CHECK_NOTHROW(static_cast<void>(
                    sdk.get_tool_path(static_cast<Sdk::Tool>(t), true)));
    }
    for (size_t j{}; j != size<Sdk::Jar>(); ++j) {
      CHECK_NOTHROW(static_cast<void>(
                    sdk.get_jar_path(static_cast<Sdk::Jar>(j), true)));
    }
    for (size_t f{}; f != size<Sdk::File>(); ++f) {
      CHECK_NOTHROW(static_cast<void>(
                    sdk.get_file_path(static_cast<Sdk::File>(f), true)));
    }
  }
}
