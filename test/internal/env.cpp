/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <cstdlib>
#include "internal/env.hpp"

using namespace std;
using namespace filesystem;

void Env::setup(const directory_entry& t_base_dir) {
  setenv("HOME", t_base_dir.path().c_str(), 1);
  unset_xdg_vars();
}

void Env::unset_xdg_vars() {
  const auto vars = {
    "XDG_CONFIG_HOME",
    "XDG_DATA_HOME",
    "XDG_CACHE_HOME"
  };

  if (s_xdg_vars_unset) {
    return;
  }
  for (const auto& v : vars) {
    unsetenv(v);
  }
  s_xdg_vars_unset = true;
}
