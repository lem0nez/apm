/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include <cpr/cprtypes.h>
#include <cpr/response.h>
#include <fcli/progress.hpp>

#include "enum_array.hpp"

class Utils {
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
  [[nodiscard]] static auto request_confirm(
      std::optional<bool> default_answer = {},
      std::istream& istream = std::cin) -> bool;

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
      const std::filesystem::path&) -> std::string;
};
