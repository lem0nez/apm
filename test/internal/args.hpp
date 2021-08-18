/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <initializer_list>
#include <string>
#include <vector>

// Provides arguments that can be passed directly
// (via getters) to the Apm's run function.
class Args {
public:
  Args(std::initializer_list<std::string>);
  // Return by reference so the run function can change a value.
  [[nodiscard]] inline auto get_argc() -> int& { return m_argc; }
  [[nodiscard]] inline auto get_argv() { return m_argv.data(); }

private:
  int m_argc;
  std::vector<std::string> m_str_argv;
  // Stores pointers to strings data of the previous container.
  std::vector<char*> m_argv;
};
