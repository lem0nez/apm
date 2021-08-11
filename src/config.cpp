/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <cstdlib>
#include <stdexcept>
#include <system_error>

#include <fcli/terminal.hpp>

#include "config.hpp"

using namespace std;

Config::Config() {
  using namespace filesystem;
  using namespace pugi;
  using namespace fcli;

  auto env_dir{getenv("XDG_CONFIG_HOME")};
  path dir;

  if (env_dir != nullptr) {
    dir = env_dir;
  } else {
    env_dir = getenv("HOME");
    if (env_dir == nullptr) {
      throw runtime_error("HOME isn't set");
    }
    dir = path(env_dir) / ".config";
  }

  error_code err;
  // If directories already exist, the function does nothing.
  create_directories(dir, err);
  if (err) {
    throw filesystem_error("failed to create directory \"" +
                           dir.string() + '"', err);
  }

  m_file_path = dir / FILE_NAME;
  const auto file_exists{exists(m_file_path, err)};
  if (err) {
    throw filesystem_error("failed to check if file \"" +
                           m_file_path.string() + "\" exists", err);
  }

  const string root_node_name(ROOT_NODE_NAME);
  if (!file_exists) {
    m_doc.append_child(root_node_name.c_str());
    if (!save()) {
      throw runtime_error(
          "failed to save file \"" + m_file_path.string() + '"');
    }
  }

  const auto parse_result{m_doc.load_file(m_file_path.c_str())};
  if (parse_result.status != status_ok) {
    throw runtime_error("failed to parse file \"" + m_file_path.string() +
        "\" (" + parse_result.description() + "); delete or fix it");
  }

  m_root_node = m_doc.child(root_node_name.c_str());
  if (!m_root_node) {
    m_root_node = m_doc.append_child(root_node_name.c_str());
    if (!m_root_node) {
      throw runtime_error("failed to create root node \"" +
                          root_node_name + '"');
    }
  }

  const optional theme{get<Theme::Name>(Key::THEME)};
  bool theme_applied{};

  if (!theme) {
    const optional term_colors{Terminal::get_cached_colors_support()};
    theme_applied = apply<Theme::Name>(Key::THEME,
        (term_colors && term_colors == Terminal::ColorsSupport::HAS_256_COLORS) ?
        DEFAULT_256COLOR_THEME : Theme::Name::DEFAULT);
  } else {
    theme_applied = apply<Theme::Name>(Key::THEME, *theme, false);
  }

  if (!theme_applied) {
    throw runtime_error("failed to apply a theme");
  }
}
