/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <fcli/progress.hpp>
#include <fcli/terminal.hpp>

#include <cpr/response.h>
#include <libzippp/libzippp.h>
#include <pugixml.hpp>

#include "config.hpp"
#include "tmp_file.hpp"

/*
 * This class has two aims:
 * 1. download and install files, that required to build application;
 * 2. provide paths to this files.
 */
class Sdk {
public:
  enum class Tool {
    // Compiles and packages APK's resources.
    AAPT2,
    // Aligns an APK file (that has ZIP format) to reduce app's memory usage.
    ZIPALIGN,

    _COUNT
  };

  enum class Jar {
    APKSIGNER,
    // Compiles Java bytecode to Android-compatible DEX bytecode.
    D8,
    // Bootstrap file that used by Java compiler.
    FRAMEWORK,

    _COUNT
  };

  enum class File {
    // Java KeyStore to sign an APK file for debugging.
    DEBUG_KEYSTORE,
    PROJECT_TEMPLATE,
    // aapt2's dependency.
    TZDATA,

    _COUNT
  };

  // Assigns (doesn't create) the root directory path.
  Sdk();
  // Starts interactive installation process. If SDK already installed,
  // then API version should be passed. Returns exit status.
  auto install(std::shared_ptr<Config> config,
      const fcli::Terminal& term, unsigned short installed_api = {},
      std::istream& istream = std::cin) -> int;

  // These functions don't guarantee that a file exists.
  [[nodiscard]] auto get_tool_path(Tool) const -> std::filesystem::path;
  [[nodiscard]] auto get_jar_path(Jar) const -> std::filesystem::path;
  [[nodiscard]] auto get_file_path(File) const -> std::filesystem::path;

private:
  static constexpr std::string_view
      ROOT_DIR_NAME{"apm"},
      TOOLS_SUBDIR_NAME{"bin"},
      JARS_SUBDIR_NAME{"lib"};

  static constexpr std::string_view
      REPO_RAW_URL_PREFIX{"https://github.com/lem0nez/apm/raw/data/"};
  static constexpr unsigned short INSTALL_MAX_PROGRESS_WIDTH{60U};

  // Returns empty document on failure.
  [[nodiscard]] static auto download_manifest(
      unsigned short progress_width) -> pugi::xml_document;
  // Returns zero on failure.
  [[nodiscard]] static auto request_api(const pugi::xml_document& manifest,
      std::istream& istream = std::cin) -> unsigned short;
  // Throws an exception of failure.
  void create_dirs() const;

  /*
   * These install functions returns false on failure. Stream of the temporary
   * file must be opened in binary output mode before calling each of function.
   * Passing API version as string, since all functions use this number only in
   * strings.
   */

  using api_dependent_install_func_t = auto (const pugi::xml_document& manifest,
      const std::string& api, TmpFile<std::ofstream>& tmp_file,
      unsigned short progress_width) const -> bool;
  api_dependent_install_func_t
      install_tools,
      install_build_tools,
      install_framework;

  auto install_tzdata(
      const pugi::xml_document& manifest, TmpFile<std::ofstream>& tmp_file,
      unsigned short progress_width) const -> bool;
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  auto install_assets(const pugi::xml_document& manifest,
      unsigned short progress_width) const -> bool;

  /*
   * If HTTP status isn't OK, this function hides progress, prints the failure
   * message and returns false. fail_using_progress determines whether to use
   * the Progress's finish function or cerr to print the failure message.
   */
  [[nodiscard]] static auto check_response(const cpr::Response& response,
      std::string_view failure_msg, fcli::Progress& progress,
      bool fail_using_progress = true) -> bool;
  // Gets value of sha256 attribute. Returns empty string on failure.
  [[nodiscard]] static auto get_sha256(const pugi::xml_node&) -> std::string;
  /*
   * Sets progress as undetermined, changes its text, compares checksums and if
   * they are not equal, finishes progress with the failure message and returns
   * false.
   */
  [[nodiscard]] static auto check_sha256(
      const std::filesystem::path& file_path, std::string_view checksum,
      fcli::Progress& progress, std::string_view failure_msg) -> bool;
  // Do the same as check_response and check_sha256. Subject used in the
  // failure messages (that print using the Progress's finish function).
  [[nodiscard]] static auto check_response_and_sha256(
      const cpr::Response& response, const std::filesystem::path& file_path,
      std::string_view checksum, std::string_view subject,
      fcli::Progress& progress) -> bool;
  // Before extracting updates text of progress. On failure
  // finishes progress with the failure message and returns false.
  [[nodiscard]] static auto extract_zip_entry(
      const libzippp::ZipEntry& entry, const std::filesystem::path& output_path,
      std::string_view name, fcli::Progress& progress) -> bool;

  std::filesystem::path m_root_dir_path;
};
