/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <filesystem>
#include <memory>
#include <system_error>

#include <cxxopts.hpp>
#include <fcli/terminal.hpp>

#include "config.hpp"
#include "sdk.hpp"

class Apm {
public:
  Apm() = delete;
  // If the constructor reproduce a error, then
  // NO operations on an instance must be performed.
  explicit Apm(std::error_condition&);
  // Passing argc by reference as cxxopts will remove all recognized arguments.
  auto run(int& argc, char** argv) -> int;

  // Returns false if wrong number is passed.
  static auto set_colors(unsigned short num) -> bool;
  // Throws an exception on failure.
  void set_release_jks(const std::filesystem::path&) const;
  void request_theme();
  void print_versions() const;

  [[nodiscard]] inline auto get_term() const -> const auto& { return m_term; }
  [[nodiscard]] inline auto get_config() const { return m_config; }
  [[nodiscard]] inline auto get_sdk() const -> std::shared_ptr<const Sdk>
      { return m_sdk; }

private:
  cxxopts::Options m_opts;
  fcli::Terminal m_term;
  // Using pointers, since exceptions, that
  // default constructors throw, must be caught.
  std::shared_ptr<Config> m_config;
  std::shared_ptr<Sdk> m_sdk;
};
