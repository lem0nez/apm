/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string_view>
#include <type_traits>

#include <fcli/theme.hpp>
#include <pugixml.hpp>

#include "enum_array.hpp"

class Config {
public:
  enum class Key {
    THEME,
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
  // it slower than passing by value. Function returns false on fail.
  template<class T,
      class V = std::conditional_t<std::is_fundamental_v<T>, const T, const T&>>
  auto apply(Key key, V val, bool save_file = true) -> bool;
  template<class T> [[nodiscard]] auto get(Key key) const -> std::optional<T>;

  // NOLINTNEXTLINE(modernize-use-nodiscard)
  inline auto save() const -> bool
      { return m_doc.save_file(m_file_path.c_str(), {}, pugi::format_raw); }

private:
  static constexpr std::string_view
      FILE_NAME{"apm.xml"},
      ROOT_NODE_NAME{"config"};
  static constexpr auto DEFAULT_256COLOR_THEME{fcli::Theme::Name::ARCTIC_DARK};

  // Since this is a constexpr function, we can return a string_view object.
  [[nodiscard]] static constexpr auto get_key_name(const Key key) {
    return EnumArray<Key, std::string_view>({
      "theme"
    }).get(key);
  }

  std::filesystem::path m_file_path;
  pugi::xml_document m_doc;
  pugi::xml_node m_root_node;
};

#include "config.inl"
