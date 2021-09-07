/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <array>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <cpr/cprtypes.h>
#include <cpr/status_codes.h>
#include <fcli/progress.hpp>

#include <doctest/doctest.h>
#include "internal/alt_stream.hpp"
#include "internal/tmp_dir.hpp"

#include "tmp_file.hpp"
#include "utils.hpp"

using namespace std;
using namespace filesystem;
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

TEST_CASE("Execute commands") {
  CHECK(Utils::exec({"echo"}) == 0);

  const TmpDir tmp_dir;
  const auto
      tmp_dir_path{tmp_dir.get_entry().path()},
      tmp_subdir_path{tmp_dir_path / "subdir"};

  create_directory(tmp_subdir_path);
  const directory_entry tmp_subdir(tmp_subdir_path);
  current_path(tmp_dir_path);

  CHECK(Utils::exec({"pwd"}, [&tmp_subdir_path] (const string_view out) {
    CHECK(tmp_subdir_path == out);
  }, {}, tmp_subdir) == 0);

  bool err_obtained{};
  CHECK(Utils::exec({"ls", "--unknown-option"}, {},
      [&err_obtained] (const string_view err) {
    if (!err.empty()) {
      err_obtained = true;
    }
  }, tmp_subdir) != 0);
  CHECK(err_obtained);

  CHECK_THROWS_AS(Utils::exec({""}, {}, {}, tmp_subdir), runtime_error);
  // Working directory must be restored in any way.
  CHECK(current_path() == tmp_dir_path);

  const vector<string> cmd{
    "sh", "-c",
    R"(printf '\n \n0\t'; printf >&2 'aZ\n)" "\u03b1\u03b2"  R"(\n\n')"
  };
  constexpr array
      expected_out{"", " ", "0\t"},
      expected_err{"aZ", "\u03b1\u03b2", ""};
  size_t
      out_line{},
      err_line{};

  CHECK(Utils::exec(cmd, [&] (const string_view out) {
    CHECK(out == expected_out.at(out_line++));
  }, [&] (const string_view err) {
    CHECK(err == expected_err.at(err_line++));
  }) == 0);
}
