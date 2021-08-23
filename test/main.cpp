/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#define DOCTEST_CONFIG_IMPLEMENT

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>

#include <cxxopts.hpp>
#include <doctest/doctest.h>

#include "internal/env.hpp"

auto main(int argc, char* argv[]) -> int {
  using namespace std;
  using namespace filesystem;
  using namespace cxxopts;

  doctest::Context context(argc, argv);
  // Preserve original arguments as they will be modified by cxxopts.
  const set<string> args(argv + 1, argv + argc);
  Options custom_opts({});

  custom_opts.allow_unrecognised_options();
  custom_opts.add_options()
      ("sdk-home", "Set HOME directory where SDK is installed", value<path>());
  const auto parsed_opts{custom_opts.parse(argc, argv)};

  if (parsed_opts.count("sdk-home") != 0U) {
    const directory_entry sdk_home(parsed_opts["sdk-home"].as<path>());
    if (!sdk_home.is_directory()) {
      cerr << '"' + sdk_home.path().string() + "\" isn't a directory!" << endl;
      return EXIT_FAILURE;
    }
    Env::set_sdk_home(sdk_home);
  } else {
    // Persistent TODO: exclude SDK dependent test cases.
    context.addFilter("test-case-exclude", "Create projects");
  }

  const auto status{context.run()};
  if (context.shouldExit()) {
    if (args.count("-?") != 0U ||
        args.count("-h") != 0U || args.count("--help") != 0U) {
      cout <<
R"(To run test cases that depend on SDK, you must specify HOME directory where
it's installed, using the custom --sdk-home option. This directory will be
assigned to the HOME environment variable.

Hint: to install SDK to <dir>, use the following command:
$ env --ignore-environment HOME=<dir> <apm executable> --set-up)" << endl;
    }
  }
  return status;
}
