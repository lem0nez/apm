/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <doctest/doctest.h>
#include "tmp_file.hpp"

using namespace std;

TEST_CASE("Create a temporary file") {
  using namespace filesystem;

  unique_ptr<TmpFile<ofstream>> file;
  REQUIRE_NOTHROW(file = make_unique<TmpFile<ofstream>>());
  const auto path{file->get_path()};
  CHECK(exists(path));

  file->get_stream() << "content" << flush;
  ifstream ifs(path);
  string content{istreambuf_iterator(ifs), istreambuf_iterator<char>()};
  ifs.close();
  CHECK(content == "content");

  file.reset();
  CHECK_FALSE(exists(path));
}
