/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

class TmpDir {
public:
  TmpDir();
  ~TmpDir();

  // Instances can share the same directory between each other.
  TmpDir(const TmpDir&) = default;
  auto operator=(const TmpDir&) -> TmpDir& = default;

  TmpDir(TmpDir&&) = default;
  auto operator=(TmpDir&&) -> TmpDir& = default;

  [[nodiscard]] inline auto get_entry() const { return *m_dir; }
  [[nodiscard]] inline auto str() const { return m_dir->path().string(); }

private:
  // ‘X’ characters will be replaced by the mkdtemp function.
  static constexpr std::string_view NAME_TEMPLATE{"apm-test_XXXXXX"};
  std::shared_ptr<const std::filesystem::directory_entry> m_dir;
};
