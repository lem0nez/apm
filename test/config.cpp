/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <doctest/doctest.h>
#include <fcli/theme.hpp>

#include "config.hpp"
#include "internal/env.hpp"
#include "internal/tmp_dir.hpp"

using namespace std;
using namespace filesystem;

TEST_CASE("Config constructor") {
  Env::unset("XDG_CONFIG_HOME");
  Env::unset("HOME");
  CHECK_THROWS_AS(Config(), runtime_error);

  {
    const TmpDir home_dir;
    permissions(home_dir.get_entry(), perms::all, perm_options::remove);
    Env::set("HOME", home_dir.str());
    CHECK_THROWS_AS(Config(), filesystem_error);
  }
  Env::unset("HOME");

  const TmpDir config_dir;
  Env::set("XDG_CONFIG_HOME", config_dir.str());

  {
    const Config config;
    // Constructor should create a configuration file.
    REQUIRE_FALSE(filesystem::is_empty(config_dir.get_entry()));
  }

  for (const auto& e : directory_iterator(config_dir.get_entry())) {
    if (e.is_regular_file()) {
      // Make a XML file not valid.
      ofstream(e.path());
    }
  }

  CHECK_THROWS_AS(Config(), runtime_error);
  Env::unset("XDG_CONFIG_HOME");
}

TEST_CASE("Apply configurations") {
  using namespace fcli;

  const TmpDir home_dir;
  Env::setup(home_dir.get_entry());

  // Destructor should save changes to a file.
  REQUIRE(Config().apply<Theme::Name>(
          Config::Key::THEME, Theme::Name::MATERIAL_LIGHT, false));
  CHECK(Config().get<Theme::Name>(Config::Key::THEME) ==
        Theme::Name::MATERIAL_LIGHT);

  // Try another keys and value types.
  Config config;

  REQUIRE(config.apply<unsigned short>(Config::Key::SDK, 1U));
  CHECK(config.get<unsigned short>(Config::Key::SDK) == 1U);

  REQUIRE(config.apply<string_view>(Config::Key::JKS_PATH, "path"));
  CHECK(config.get<path>(Config::Key::JKS_PATH) == "path");

  config.unbind_file();
  REQUIRE(config.apply<bool>(Config::Key::JKS_KEY_HAS_PASSWORD, true));
  CHECK(*config.get<bool>(Config::Key::JKS_KEY_HAS_PASSWORD));
}

TEST_CASE("Remove configurations") {
  const TmpDir home_dir;
  Env::setup(home_dir.get_entry());

  Config config;
  // Non-existent configuration.
  CHECK(config.remove(Config::Key::THEME));

  REQUIRE(config.apply<string>(Config::Key::JKS_KEY_ALIAS, "name"));
  REQUIRE(config.remove(Config::Key::JKS_KEY_ALIAS));
  CHECK_FALSE(config.get<string>(Config::Key::JKS_KEY_ALIAS).has_value());
}
