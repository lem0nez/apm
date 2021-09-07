/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <cpr/api.h>
#include <cpr/callback.h>

#include <fcli/terminal.hpp>
#include <fcli/text.hpp>

#include <openssl/sha.h>
#include <pstreams/pstream.h>

#include "general/scope_guard.hpp"
#include "utils.hpp"

using namespace std;
using namespace filesystem;

using namespace cpr;
using namespace fcli;
using namespace fcli::literals;

auto Utils::request_confirm(optional<bool> t_default_answer) -> bool {
  // Maximum characters to be converted to lowercase.
  constexpr size_t MAX_CONVERT_CHARS{3U};
  string answer;

  string formatted_request_msg;
  const optional term_colors{Terminal::get_cached_colors_support()};

  if (t_default_answer) {
    // Emphasize or mark with the asterisk symbol default answer.
    if (*t_default_answer) {
      if (term_colors) {
        formatted_request_msg = "<u>y~d~es<r>/n~d~o<r>> <b>"_fmt;
      } else {
        formatted_request_msg = "yes*/no> ";
      }
    } else if (term_colors) {
      formatted_request_msg = "y~d~es<r>/<u>n~d~o<r>> <b>"_fmt;
    } else {
      formatted_request_msg = "yes/no*> ";
    }
  } else {
    formatted_request_msg = "y~d~es<r>/n~d~o<r>> <b>"_fmt;
  }

  while (true) {
    cout << formatted_request_msg << flush;
    if (t_default_answer) {
      // Allow empty input using getline.
      getline(cin, answer);
    } else {
      cin >> answer;
      ignore_cin_line();
    }
    cout << "<r>"_fmt << flush;

    if (!check_cin()) {
      continue;
    }
    if (t_default_answer && answer.empty()) {
      return *t_default_answer;
    }

    // Convert only part that required for comparison.
    const auto convert_chars_count{min(answer.size(), MAX_CONVERT_CHARS)};
    for (size_t i{}; i != convert_chars_count; ++i) {
      answer.at(i) = static_cast<char>(tolower(answer.at(i)));
    }

    if (answer == "y" || answer == "yes") {
      return true;
    }
    if (answer == "n" || answer == "no") {
      return false;
    }
    cerr << Text::format_message(Text::Message::ERROR,
            R"(Wrong answer! Enter "yes" or "no")") << endl;
  }
}

auto Utils::check_cin() -> bool {
  if (cin) {
    return true;
  }

  cerr << "Invalid input! Try again"_err << endl;
  cin.clear();
  ignore_cin_line();
  return false;
}

auto Utils::download(ofstream& t_ofs, const Url& t_url,
    Progress& t_progress, const bool t_append_size) -> Response {
  using namespace chrono;
  using namespace chrono_literals;

  // Since the progress callback calls too often, we need
  // to limit how often to update the progress bar.
  constexpr auto REFRESH_INTERVAL{100ms};
  time_point<steady_clock>
      prev_refresh,
      current_time;

  const auto original_text{t_progress.get_text()};
  // Will contain the total file size (if progress is determined).
  string size_postfix;

  ostringstream size_oss;
  size_oss << fixed;
  size_oss.precision(1);

  const auto get_size_mb{[&size_oss] (const size_t bytes) {
    size_oss.str({});
    size_oss << static_cast<double>(bytes) / pow(1024, 2);
    return size_oss.str();
  }};

  const ProgressCallback progress_callback([&]
      (const size_t download_total, const size_t downloaded,
      size_t /* upload_total */, size_t /* uploaded */) {
    if (download_total == 0U) {
      return true;
    }

    current_time = steady_clock::now();
    if (current_time - prev_refresh < REFRESH_INTERVAL) {
      return true;
    }
    prev_refresh = current_time;

    if (t_progress.is_determined()) {
      t_progress = static_cast<double>(downloaded * 100U) / download_total;
    }

    if (!t_append_size) {
      return true;
    }
    if (t_progress.is_determined()) {
      if (size_postfix.empty()) {
        size_postfix = " / " + get_size_mb(download_total) + " MB)";
      }
      t_progress =
          original_text + " (" + get_size_mb(downloaded) + size_postfix;
    } else {
      t_progress = original_text + " (" + get_size_mb(downloaded) + " MB)";
    }
    return true;
  });

  const auto response{Download(t_ofs, t_url, progress_callback)};
  if (t_append_size) {
    t_progress = original_text;
  }
  return response;
}

auto Utils::calc_sha256(const path& t_path) -> string {
  constexpr size_t BUFFER_SIZE{1U << 13U};

  ifstream ifs(t_path, ios::binary);
  if (!ifs) {
    return {};
  }

  SHA256_CTX context;
  if (SHA256_Init(&context) != 1) {
    return {};
  }

  array<char, BUFFER_SIZE> buf{};
  do {
    ifs.read(buf.data(), buf.size());
    if (SHA256_Update(&context, buf.data(),
        static_cast<unsigned long>(ifs.gcount())) != 1) {
      return {};
    }
  } while (ifs);

  array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
  if (SHA256_Final(hash.data(), &context) != 1) {
    return {};
  }

  ostringstream result_oss;
  result_oss << hex << setfill('0');
  for (const auto& c : hash) {
    result_oss << setw(2) << static_cast<unsigned short>(c);
  }
  return result_oss.str();
}

auto Utils::exec(const vector<string>& t_cmd,
                 const function<output_callback_t>& t_out_callback,
                 const function<output_callback_t>& t_err_callback,
                 const directory_entry& t_work_dir) -> int {
  using namespace redi;
  constexpr size_t OUTPUT_BUFFER_SIZE{512U};

  ScopeGuard work_path_guard;
  if (!t_work_dir.path().empty()) {
    auto prev_work_path{current_path()};
    current_path(t_work_dir);
    work_path_guard = [path = move(prev_work_path)] { current_path(path); };
  }

  ipstream ips(t_cmd, pstreams::pstdout | pstreams::pstderr);
  if (!ips.is_open()) {
    throw runtime_error("failed to start a process");
  }

  if (t_out_callback || t_err_callback) {
    // These variables determine whether EOF of a stream has been reached.
    // Initializer excludes a stream which output isn't required.
    auto
        out_finished{!t_out_callback},
        err_finished{!t_err_callback};
    string out, err;

    array<char, OUTPUT_BUFFER_SIZE> buf{};
    streamsize extracted_chars_count{};
    bool process_extracted_chars{};

    const auto process_stream{[&] (ipstream& stream, bool& finished,
        string& output, const function<output_callback_t>& callback) {
      if (finished) {
        return;
      }

      // Read output in a non-blocking way.
      while ((extracted_chars_count =
              stream.readsome(buf.data(), buf.size())) > 0) {
        output += string(buf.cbegin(), buf.cbegin() + extracted_chars_count);
        process_extracted_chars = true;
      }

      if (process_extracted_chars) {
        process_extracted_chars = false;
        size_t new_line_pos{};
        while ((new_line_pos = output.find('\n')) != string::npos) {
          callback(output.substr(0U, new_line_pos));
          output.erase(0U, new_line_pos + 1U);
        }
      }

      if (ips.eof()) {
        // Clear eofbit.
        ips.clear();
        finished = true;
        if (!output.empty()) {
          callback(output);
        }
      }
    }};

    do {
      // By calling the out and err functions
      // we switch between the stream buffers.
      process_stream(ips.out(), out_finished, out, t_out_callback);
      process_stream(ips.err(), err_finished, err, t_err_callback);
    } while (!(out_finished && err_finished));
  }

  // Wait in a blocking way until eofbit of the streams will be set,
  // since calling only the close function may lead to an error.
  if (!t_out_callback) {
    ips.out().ignore(numeric_limits<streamsize>::max());
  }
  if (!t_err_callback) {
    ips.err().ignore(numeric_limits<streamsize>::max());
  }

  ips.close();
  const auto buf{ips.rdbuf()};
  if (!buf->exited()) {
    throw runtime_error("a process isn't exited");
  }
  return buf->status();
}

auto Utils::get_term_width(
    const Terminal& t_term, const unsigned short t_max_width,
    const unsigned short t_fall_back_width) -> unsigned short {

  if (s_term_width_status == TermWidth::NOT_CHECKED) {
    try {
      static_cast<void>(t_term.get_width());
      s_term_width_status = TermWidth::AVAILABLE;
    } catch (const runtime_error&) {
      s_term_width_status = TermWidth::NOT_AVAILABLE;
    }
  }
  return s_term_width_status == TermWidth::AVAILABLE ?
         min(t_term.get_width(), t_max_width) : t_fall_back_width;
}
