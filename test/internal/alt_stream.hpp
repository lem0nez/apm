/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <type_traits>

// O is type of original stream (istream or ostream).
// S is type of alternative string stream.
template<typename O, typename S =
    std::conditional_t<std::is_same_v<O, std::istream>,
                       std::istringstream, std::ostringstream>>
class AltStream {
public:
  // Replaces buffer of the original stream to buffer of the alternative stream.
  inline explicit AltStream(O& orig):
      m_orig_stream(orig), m_orig_buf(m_orig_stream.rdbuf())
      { m_orig_stream.rdbuf(m_alt_stream.rdbuf()); }
  // Restores buffer of the original stream.
  inline ~AltStream() { m_orig_stream.rdbuf(m_orig_buf.release()); }

  AltStream(const AltStream&) = delete;
  auto operator=(const AltStream&) -> AltStream& = delete;

  AltStream(AltStream&&) = delete;
  auto operator=(AltStream&&) -> AltStream& = delete;

  [[nodiscard]] inline auto operator*() -> S& { return m_alt_stream; }

private:
  O& m_orig_stream;
  std::unique_ptr<std::streambuf> m_orig_buf;
  S m_alt_stream;
};

template<typename O> AltStream(O&) -> AltStream<O>;
