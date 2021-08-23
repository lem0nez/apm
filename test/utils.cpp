/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <filesystem>
#include <iostream>
#include <limits>
#include <string_view>

#include <cpr/cprtypes.h>
#include <cpr/status_codes.h>
#include <fcli/progress.hpp>

#include <doctest/doctest.h>
#include "utils.hpp"

#include "internal/alt_stream.hpp"
#include "tmp_file.hpp"

using namespace std;
using namespace fcli;

TEST_CASE("Confirmation requester") {
  AltStream
      alt_cout(cout),
      alt_cerr(cerr);
  AltStream alt_cin(cin);

  // Default answer.
  (*alt_cin).str("\n");
  CHECK(Utils::request_confirm(true));
  (*alt_cin).str("\n");
  CHECK_FALSE(Utils::request_confirm(false));

  // Default answer and user input.
  (*alt_cin).str("\nn\n");
  CHECK(Utils::request_confirm(true));
  (*alt_cin).str("y\n");
  CHECK(Utils::request_confirm(false));

  // Mix of letter cases.
  (*alt_cin).str("N\n");
  CHECK_FALSE(Utils::request_confirm());
  (*alt_cin).str("nO\n");
  CHECK_FALSE(Utils::request_confirm());
  (*alt_cin).str("YeS\n");
  CHECK(Utils::request_confirm());

  REQUIRE((*alt_cerr).tellp() == streampos(0));

  // ------------- +
  // Wrong answers |
  // ------------- +

  (*alt_cin).str("nn\ny\n");
  CHECK(Utils::request_confirm());
  CHECK((*alt_cerr).tellp() > streampos(0));
  (*alt_cerr).str({});

  (*alt_cin).str("yess\nn\n");
  CHECK_FALSE(Utils::request_confirm());
  CHECK((*alt_cerr).tellp() > streampos(0));
}

TEST_CASE("Check the standard input stream") {
  AltStream alt_cerr(cerr);
  AltStream alt_cin(cin);

  CHECK(Utils::check_cin());
  CHECK((*alt_cerr).tellp() == streampos(0));

  cin.setstate(ios::failbit);
  CHECK_FALSE(Utils::check_cin());
  // An error message should be printed.
  CHECK((*alt_cerr).tellp() > streampos(0));
  // Fail state should be reset.
  CHECK(cin);
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
      EMPTY_HASH(
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
      CONTENT("content"),
      CONTENT_HASH(
          "ed7002b439e9ac845f22357d822bac1444730fbdb6016d3ec9432297b9ec9f73");

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
