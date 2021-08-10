/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include "apm.hpp"

auto main(int argc, char* argv[]) -> int {
  return Apm().run(argc, argv);
}
