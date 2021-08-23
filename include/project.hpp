/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include <fcli/terminal.hpp>

class Project {
public:
  // Returns program execution status.
  static auto create(const std::filesystem::path& dir,
      unsigned short sdk_api, const std::filesystem::path& templ_zip,
      const fcli::Terminal& term) -> int;

private:
  // Minimum value of the Android's minimum API level to run an application.
  static constexpr unsigned short MIN_API{21U};

  [[nodiscard]] static auto request_app_name() -> std::string;
  [[nodiscard]] static auto request_package() -> std::string;
  // Requests minimum API level of Android that required to run an application.
  // sdk_api is minimum API version of SDK that required to compile a project.
  [[nodiscard]] static auto
      request_min_api(unsigned short sdk_api) -> unsigned short;

  // Following functions throw an exception on failure.

  static void extract_template(const std::filesystem::path& dest,
      const std::filesystem::path& templ_zip);
  // Move, rename or delete resource files or folders according to
  // minimum API level. Using directory_entry because a directory must exist.
  static void organize_resources(
      const std::filesystem::directory_entry& res_dir, unsigned short min_api);
  // Replaces variable names with their values in template files.
  static void expand_variables(
      const std::filesystem::directory_entry& project_root,
      std::string_view package, unsigned short sdk_api, unsigned short min_api);
  static void set_app_name(
      const std::filesystem::path& strings_xml, std::string_view name);
  // Creates a folder hierarchy in root directory according
  // to package name and moves a file to the extreme folder.
  static void relocate_class(const std::filesystem::directory_entry& root_dir,
      const std::filesystem::path& file, std::string_view package);
};
