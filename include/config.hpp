/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <fcli/theme.hpp>
#include <pugixml.hpp>

#include "general/enum_array.hpp"

class Config {
public:
  enum class Key {
    THEME,
    SDK,

    // Path of the Java KeyStore file that
    // used for signing the release APK files.
    JKS_PATH,
    JKS_KEY_ALIAS,
    JKS_KEY_HAS_PASSWD,

    _COUNT
  };

  Config();
  inline ~Config() { save(); }

  // Configuration file is unique for a program.
  Config(const Config&) = delete;
  auto operator=(const Config&) -> Config& = delete;

  Config(Config&&) = default;
  auto operator=(Config&&) -> Config& = default;

  // V type: don's pass a value of fundamental type by reference as
  // it slower than passing by value. Function returns false on failure.
  template<typename T, typename V =
      std::conditional_t<std::is_fundamental_v<T>, const T, const T&>>
  auto apply(Key key, V val, bool save_file = true) -> bool;

  template<typename T>
  [[nodiscard]] auto get(Key key) const -> std::optional<T>;

  auto remove(Key key, bool save_file = true) -> bool;
  // Use it if you don't want to preserve changes to file on destruction.
  inline void unbind_file() { m_file_path.clear(); }
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  auto save() const -> bool;

private:
  static constexpr std::string_view
      FILE_NAME{"apm.xml"},
      ROOT_NODE_NAME{"config"};
  static constexpr auto DEFAULT_256COLOR_THEME{fcli::Theme::Name::ARCTIC_DARK};

  // Since this is a constexpr function, we can return a string_view object.
  [[nodiscard]] static constexpr auto get_key_name(const Key key) {
    return EnumArray<Key, std::string_view>{
      "theme", "sdk", "jks", "jks-key", "jks-key-has-passwd"
    }.get(key);
  }

  std::filesystem::path m_file_path;
  pugi::xml_document m_doc;
  pugi::xml_node m_root_node;
};

#include "config.inl"
