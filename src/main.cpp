/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <cstdlib>
#include <system_error>

#include "apm.hpp"

auto main(int argc, char* argv[]) -> int {
  std::error_condition err;
  Apm apm(err);
  if (err) {
    return err.value();
  }
  return apm.run(argc, argv);
}
