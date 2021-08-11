/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <string>

template<typename T, typename V>
// V type already has the const specifier.
auto Config::apply(const Key t_key, V t_val, const bool t_save_file) -> bool {
  using namespace std;

  string val_str;
  if constexpr (is_arithmetic_v<T>) {
    val_str = to_string(t_val);
  } else if constexpr (is_enum_v<T>) {
    val_str = to_string(static_cast<int>(t_val));
  } else {
    val_str = t_val;
  }

  if (t_key == Key::THEME) {
    try {
      fcli::Theme::set_theme(t_val);
    } catch (...) {
      return false;
    }
  }

  const string node_name(get_key_name(t_key));
  auto node{m_root_node.child(node_name.c_str())};
  if (!node) {
    if (!(node = m_root_node.append_child(node_name.c_str()))) {
      return false;
    }
  }
  if (!node.text().set(val_str.c_str())) {
    return false;
  }
  if (t_save_file) {
    return save();
  }
  return true;
}

template<typename T>
auto Config::get(const Key t_key) const -> std::optional<T> {
  using namespace std;
  optional<T> val;

  const auto node{m_root_node.child(string(get_key_name(t_key)).c_str())};
  if (node) {
    const auto text{node.text()};

    if constexpr (is_enum_v<T>) {
      val = static_cast<T>(text.as_int());
    } else if constexpr (is_floating_point_v<T>) {
      val = text.as_double();
    } else if constexpr (is_same_v<T, bool>) {
      val = text.as_bool();
    } else if constexpr (is_same_v<T, int>) {
      val = text.as_int();
    } else if constexpr (is_same_v<T, unsigned>) {
      val = text.as_uint();
    } else {
      val = text.as_string();
    }
  }
  return val;
}
