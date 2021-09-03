/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <cstdlib>
#include <stdexcept>
#include <string>

#include "internal/tmp_dir.hpp"

using namespace std;
using namespace filesystem;

TmpDir::TmpDir() {
  string path_template(temp_directory_path() / NAME_TEMPLATE);
  const auto path_ptr{mkdtemp(path_template.data())};
  if (path_ptr == nullptr) {
    throw runtime_error("failed to create a temporary directory");
  }
  m_dir = make_shared<const directory_entry>(path_ptr);
}

TmpDir::~TmpDir() {
  if (m_dir.use_count() == 1) {
    remove_all(*m_dir);
  }
}
