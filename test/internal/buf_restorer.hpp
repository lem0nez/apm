/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <ios>
#include <memory>

// Restores a stream buffer on destruct.
class BufRestorer {
public:
  explicit BufRestorer(std::ios& stream):
      m_stream(stream), m_buf(m_stream.rdbuf()) {}
  inline ~BufRestorer() { m_stream.rdbuf(m_buf.release()); }

  BufRestorer(const BufRestorer&) = delete;
  auto operator=(const BufRestorer&) -> BufRestorer& = delete;

  BufRestorer(BufRestorer&&) = delete;
  auto operator=(BufRestorer&&) -> BufRestorer& = delete;

private:
  std::ios& m_stream;
  std::unique_ptr<std::streambuf> m_buf;
};
