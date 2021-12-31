/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <functional>
#include <utility>

// An instance of this class calls the passed handler function on destruction.
class ScopeGuard {
  using handler_t = std::function<void()>;

public:
  ScopeGuard() = default;
  template<typename F> explicit ScopeGuard(F&& handler) try:
      m_handler(std::forward<F>(handler)) {} catch (...) {
    // Guarantee the handler execution even if an
    // exception was thrown during construction.
    handler();
    throw;
  }
  ~ScopeGuard() {
    if (m_handler) {
      m_handler();
    }
  }

  ScopeGuard(const ScopeGuard&) = delete;
  auto operator=(const ScopeGuard&) -> ScopeGuard& = delete;
  // There are too many subtleties to properly implement move
  // semantics of this class... So this is not so necessary. :)
  ScopeGuard(ScopeGuard&&) = delete;
  auto operator=(ScopeGuard&&) -> ScopeGuard& = delete;

  template<typename F> auto operator=(F&& handler) -> ScopeGuard& {
    try {
      m_handler = std::forward<F>(handler);
    } catch (...) {
      handler();
      throw;
    }
    return *this;
  }

private:
  handler_t m_handler;
};
