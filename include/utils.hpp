/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <cpr/cprtypes.h>
#include <cpr/response.h>

#include <fcli/progress.hpp>
#include <fcli/terminal.hpp>

#include "general/enum_array.hpp"

class Utils {
  using output_callback_t = void (std::string_view line);

public:
  enum class Arch {
    X86_64,
    I386,
    AARCH64,
    ARM,
    MIPS64,
    MIPS,

    _COUNT
  };

  [[nodiscard]] static constexpr auto get_arch() {
#ifdef __x86_64__
    return Arch::X86_64;
#elif defined(__i386__)
    return Arch::I386;
#elif defined(__aarch64__)
    return Arch::AARCH64;
#elif defined(__arm__)
    return Arch::ARM;
#elif defined(__mips64) || defined(__mips__) && INTPTR_MAX == INT64_MAX
    return Arch::MIPS64;
#elif defined(__mips__)
    return Arch::MIPS;
#else
#error Unsupported architecture
#endif
  }

  [[nodiscard]] static constexpr auto get_arch_name(const Arch arch) {
    return EnumArray<Arch, std::string_view>{
      "x86_64", "i386", "aarch64", "armel", "mips64el", "mipsel"
    }.get(arch);
  }

  // Returns true if the user answered positively, or false otherwise.
  [[nodiscard]] static auto
      request_confirm(std::optional<bool> default_answer = {}) -> bool;
  // If the standard stream has errors, then an error message
  // will be printed, stream cleared and false returned.
  static auto check_cin() -> bool;
  /*
   * This function must be called before switching to std::getline after
   * whitespace-delimited input (e. g., via operator >>). If you don't,
   * std::getline just will return immediately.
   */
  static inline auto ignore_cin_line() -> std::istream& {
    return std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  /*
   * Downloads a file with progress reporting. append_size determines
   * whether to append downloaded and total file sizes to the text of
   * progress bar (original text will be restored after downloading).
   *
   * If progress is undetermined, then total
   * file size and percents won't be reported.
   */
  static auto download(std::ofstream& ofs, const cpr::Url& url,
      fcli::Progress& progress, bool append_size = true) -> cpr::Response;

  // Returns empty string on failure.
  [[nodiscard]] static auto calc_sha256(
      const std::filesystem::path& path) -> std::string;

  /*
   * Executes a command in a new process. If a callback is provided, it will be
   * called every time when entire line retrieved or EOF of a stream reached. If
   * work_dir is provided, function will change the current working directory
   * before executing the command and then will restore it.
   */
  static auto exec(const std::vector<std::string>& cmd,
      const std::function<output_callback_t>& out_callback = {},
      const std::function<output_callback_t>& err_callback = {},
      const std::filesystem::directory_entry& work_dir = {}) -> int;

  // If unable to retrieve terminal width,
  // then fall_back_width will be returned.
  [[nodiscard]] static auto get_term_width(
      const fcli::Terminal& term, unsigned short max_width,
      unsigned short fall_back_width) -> unsigned short;

private:
  enum class TermWidth {
    NOT_CHECKED,
    AVAILABLE,
    NOT_AVAILABLE
  };
  static inline TermWidth s_term_width_status{TermWidth::NOT_CHECKED};
};
