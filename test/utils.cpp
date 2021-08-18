/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <string_view>

#include <cpr/cprtypes.h>
#include <cpr/status_codes.h>
#include <fcli/progress.hpp>
#include <fcli/terminal.hpp>

#include <doctest/doctest.h>
#include "utils.hpp"

#include "internal/alt_stream.hpp"
#include "tmp_file.hpp"

using namespace std;
using namespace fcli;

TEST_CASE("Confirmation requester") {
  istringstream alt_cin;
  AltStream
      alt_cout(cout),
      alt_cerr(cerr);

  // Default answer.
  alt_cin.str("\n");
  CHECK(Utils::request_confirm(true, alt_cin));
  alt_cin.str("\n");
  CHECK_FALSE(Utils::request_confirm(false, alt_cin));

  // Default answer and user input.
  alt_cin.str("\nn\n");
  CHECK(Utils::request_confirm(true, alt_cin));
  alt_cin.str("y\n");
  CHECK(Utils::request_confirm(false, alt_cin));

  // Mix of letter cases.
  alt_cin.str("N\n");
  CHECK_FALSE(Utils::request_confirm({}, alt_cin));
  alt_cin.str("nO\n");
  CHECK_FALSE(Utils::request_confirm({}, alt_cin));
  alt_cin.str("YeS\n");
  CHECK(Utils::request_confirm({}, alt_cin));

  REQUIRE((*alt_cerr).tellp() == streampos(0));

  /*
   * Wrong answers.
   */

  alt_cin.str("nn\ny\n");
  CHECK(Utils::request_confirm({}, alt_cin));
  CHECK((*alt_cerr).tellp() > streampos(0));
  (*alt_cerr).str({});

  alt_cin.str("yess\nn\n");
  CHECK_FALSE(Utils::request_confirm({}, alt_cin));
  CHECK((*alt_cerr).tellp() > streampos(0));
}

TEST_CASE("Download a file") {
  using namespace cpr;

  Progress progress({}, true, cout);
  TmpFile file;
  auto& ofs{file.get_stream()};
  const Url url("https://github.com/lem0nez/apm/raw/data/manifest.xml");

  REQUIRE(Utils::download(ofs, url, progress).status_code == status::HTTP_OK);
  CHECK(progress.get_percents() > numeric_limits<double>::epsilon());
  CHECK(ofs.tellp() > streampos(0));
}

TEST_CASE("Calculate SHA256") {
  using namespace filesystem;
  constexpr string_view
      EMPTY_HASH
          {"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
      CONTENT{"content"},
      CONTENT_HASH
          {"ed7002b439e9ac845f22357d822bac1444730fbdb6016d3ec9432297b9ec9f73"};

  TmpFile file;
  auto& ofs{file.get_stream()};
  const auto& path{file.get_path()};
  CHECK(Utils::calc_sha256(path) == EMPTY_HASH);

  ofs << CONTENT << flush;
  CHECK(Utils::calc_sha256(path) == CONTENT_HASH);

  ofs.close();
  remove(path);
  CHECK(Utils::calc_sha256(path).empty());
}

TEST_CASE("Get terminal width") {
  constexpr unsigned short
      MAX_WIDTH{10U},
      FALL_BACK_WIDTH{100U};

  // Pass invalid file descriptor.
  CHECK(Utils::get_term_width(
      Terminal(-1), MAX_WIDTH, FALL_BACK_WIDTH) == FALL_BACK_WIDTH);
}
