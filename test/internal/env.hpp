/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once
#include <filesystem>

class Env {
public:
  // Sets base directory for the program so
  // as not to pollute the user environment.
  static void setup(const std::filesystem::directory_entry& base_dir);

private:
  static void unset_xdg_vars();
  static inline bool s_xdg_vars_unset;
};
