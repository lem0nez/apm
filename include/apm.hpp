/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <cstdint>
#include <cxxopts.hpp>
#include <memory>

#include "config.hpp"

class Apm {
public:
  enum class Arch {
    X86_64,
    I386,
    AARCH64,
    ARM,
    MIPS64,
    MIPS
  };

  Apm();
  // Passing argc by reference as cxxopts will remove all recognized arguments.
  auto run(int& argc, char** argv) -> int;

  void request_theme();
  void print_versions() const;

  [[nodiscard]] static constexpr auto get_arch() -> Arch {
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

private:
  cxxopts::Options m_opts;
  // Using a pointer, since exceptions, that
  // default constructor throws, must be caught.
 std::unique_ptr<Config> m_config;
};
