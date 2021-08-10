/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include "internal/args.hpp"

using namespace std;

Args::Args(initializer_list<string> t_argv):
           m_argc(static_cast<int>(t_argv.size())), m_str_argv(t_argv) {
  for (auto& s : m_str_argv) {
    m_argv.push_back(s.data());
  }
}
