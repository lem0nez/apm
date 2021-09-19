/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <fcli/terminal.hpp>
#include <pugixml.hpp>

#include "apm.hpp"
#include "jvm.hpp"

class Project {
public:
  enum class AppDir {
    ROOT,
    ASSETS,
    RESOURCES,
    JAVA_SRC,

    _COUNT
  };

  enum class BuildDir {
    // Resource files compiled to the .flat format by aapt2.
    FLAT_RESOURCES,
    APKS,
    // Java class “R” generated by aapt2.
    R_JAVA,
    JAVA_CLASSES,
    // Each DEX is a Java class.
    INTERMEDIATE_DEXES,
    // Final DEX files to be added to the APK archive.
    DEXES,

    _COUNT
  };

  enum class BuildConfig {
    DEBUG,
    RELEASE,
    // Represents both DEBUG and RELEASE.
    ALL,

    _COUNT
  };

  enum class ApkType {
    BASE,
    SIGNED,
    ALIGNED,
    FINAL,

    _COUNT
  };

  Project() = default;
  // Throws an exception on failure.
  explicit Project(const std::filesystem::path& root_dir);
  // To build a project, instance must be initialized with a path. Returns
  // program execution status. NOLINTNEXTLINE(modernize-use-nodiscard)
  auto build(const Apm& apm, bool is_debug_build,
             const std::filesystem::path& output_apk_copy = {},
             std::shared_ptr<const Jvm> jvm = {}) const -> int;

  [[nodiscard]] auto get_app_dir(AppDir dir) const -> std::filesystem::path;
  [[nodiscard]] auto get_build_dir(BuildDir dir, BuildConfig config,
      bool auto_create = true) const -> std::filesystem::path;
  [[nodiscard]] auto get_apk_path(ApkType type, BuildConfig build_config,
      bool auto_create_parent_dir = true) const -> std::filesystem::path;

  // Returns program execution status.
  static auto create(const std::filesystem::path& dir,
      unsigned short sdk_api, const std::filesystem::path& templ_zip,
      const fcli::Terminal& term) -> int;

private:
  static constexpr std::string_view CONFIG_FILE_NAME{"apm.xml"};
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

  std::filesystem::directory_entry m_dir;
  // Contains the root document element.
  pugi::xml_node m_config_root;
};