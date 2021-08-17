/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>

template <typename T> TmpFile<T>::TmpFile(const std::ios::openmode t_openmode) {
  using namespace std;
  // Using string instead of path so the mkstemp function can modify data.
  string path{filesystem::temp_directory_path() / NAME_TEMPLATE};
  const auto fd{mkstemp(path.data())};

  if (fd == -1) {
    throw runtime_error("failed to create a temporary file");
  }

  m_path = path;
  if (t_openmode == ios::openmode{}) {
    m_stream.open(m_path);
  } else {
    m_stream.open(m_path, t_openmode);
  }
  close(fd);
}

template<typename T> TmpFile<T>::~TmpFile() {
  using namespace std;

  m_stream.close();
  error_code err;
  // Using a non-throwing function, since we're inside the destructor.
  filesystem::remove(m_path, err);
}
