/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include "jvm.hpp"

class Env {
public:
  // Sets home directory for a program so
  // as not to pollute the user environment.
  static void setup(const std::filesystem::directory_entry& home_dir);

  // Wrappers around setenv and unsetenv that throw exceptions.
  static void set(std::string_view var_name, std::string_view val);
  static void unset(std::string_view var_name);

  static inline void set_sdk_home(const std::filesystem::directory_entry& dir)
      { s_sdk_home = dir; }
  [[nodiscard]] static inline auto get_sdk_home() { return s_sdk_home; }

  // Share single JVM between test cases, since we can't instantiate more
  // then one per a process. To call this function, HOME of SDK must be set.
  [[nodiscard]] static auto get_jvm() -> std::shared_ptr<Jvm>;
  static inline auto release_jvm() { s_jvm.reset(); }

private:
  static void unset_xdg_vars();
  // Environment variables should be unset only once for a program.
  static inline bool s_xdg_vars_unset;

  static inline std::filesystem::directory_entry s_sdk_home;
  static inline std::shared_ptr<Jvm> s_jvm;
};
