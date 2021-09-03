/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <array>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "sdk.hpp"
#include "internal/env.hpp"

using namespace std;
using namespace filesystem;

void Env::setup(const directory_entry& t_home_dir) {
  set("HOME", t_home_dir.path().string());
  unset_xdg_vars();
}

void Env::set(const string_view t_var_name, const string_view t_val) {
  const auto result
      {setenv(string(t_var_name).c_str(), string(t_val).c_str(), 1)};
  if (result != 0) {
    throw runtime_error("failed to set the \"" + string(t_var_name) +
                        "\" environment variable");
  }
}

void Env::unset(const string_view t_var_name) {
  if (unsetenv(string(t_var_name).c_str()) != 0) {
    throw runtime_error("failed to unset the \"" + string(t_var_name) +
                        "\" environment variable");
  }
}

auto Env::get_jvm() -> shared_ptr<Jvm> {
  if (!s_jvm) {
    const auto home{getenv("HOME")};
    setup(s_sdk_home);
    s_jvm = make_shared<Jvm>(jvm_tools::ALL, make_shared<const Sdk>());
    if (home != nullptr) {
      set("HOME", home);
    }
  }
  return s_jvm;
}

void Env::unset_xdg_vars() {
  constexpr array names{
    "XDG_CONFIG_HOME",
    "XDG_DATA_HOME",
    "XDG_CACHE_HOME"
  };

  if (s_xdg_vars_unset) {
    return;
  }
  for (const auto& n : names) {
    unset(n);
  }
  s_xdg_vars_unset = true;
}
