/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <string_view>
#include <type_traits>

template<typename T = std::ofstream> class TmpFile {
public:
  static_assert(std::is_base_of_v<std::fstream, T> ||
                std::is_base_of_v<std::ofstream, T> ||
                std::is_base_of_v<std::ifstream, T>,
                "T must be a file stream");
  // Creates a temporary file and opens a stream. Can throw an exception.
  // If no flags are specified then default openmode for T will be used.
  explicit TmpFile(std::ios::openmode openmode = {});
  // Closes the stream and deletes the file.
  ~TmpFile();

  // Streams can't be copied.
  TmpFile(const TmpFile&) = delete;
  auto operator=(const TmpFile&) -> TmpFile& = delete;

  /*
   * We can't make them noexcept, because move constructor and
   * move assignment operator of file stream classes throw exceptions.
   */
  // NOLINTNEXTLINE(hicpp-noexcept-move)
  TmpFile(TmpFile&&) = default;
  // NOLINTNEXTLINE(hicpp-noexcept-move)
  auto operator=(TmpFile&&) -> TmpFile& = default;

  [[nodiscard]] inline auto get_stream() -> auto& { return m_stream; }
  [[nodiscard]] inline auto get_path() const { return m_path; }

private:
  static constexpr std::string_view NAME_TEMPLATE{"apm-XXXXXX"};

  // Store a path as it required to delete the file.
  std::filesystem::path m_path;
  T m_stream;
};

#include "tmp_file.inl"
