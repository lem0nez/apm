/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <doctest/doctest.h>
#include <fcli/theme.hpp>

#include "config.hpp"
#include "internal/tmp_dir.hpp"

using namespace std;

TEST_CASE("Constructor") {
  using namespace filesystem;

  unsetenv("XDG_CONFIG_HOME");
  unsetenv("HOME");
  CHECK_THROWS_AS(Config(), runtime_error);

  const TmpDir config_dir;
  setenv("XDG_CONFIG_HOME", config_dir.c_str(), 1);

  {
    const Config config;
    // Constructor should create a configuration file.
    REQUIRE_FALSE(filesystem::is_empty(config_dir.get_entry()));
  }

  for (const auto& e : recursive_directory_iterator(config_dir.get_entry())) {
    if (e.is_regular_file()) {
      // Make a XML file not valid.
      ofstream(e.path());
    }
  }
  CHECK_THROWS_AS(Config(), runtime_error);
}

TEST_CASE("Apply theme") {
  using namespace fcli;

  const TmpDir home_dir;
  unsetenv("XDG_CONFIG_HOME");
  setenv("HOME", home_dir.c_str(), 1);

  // Destructor should save changes to a file.
  REQUIRE(Config().apply<Theme::Name>(
          Config::Key::THEME, Theme::Name::MATERIAL_LIGHT, false));
  CHECK(Config().get<Theme::Name>(Config::Key::THEME) ==
        Theme::Name::MATERIAL_LIGHT);
}
