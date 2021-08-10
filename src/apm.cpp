/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <array>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <utility>

#include <fcli/terminal.hpp>
#include <fcli/text.hpp>
#include <fcli/theme.hpp>

#include "apm.hpp"

using namespace std;
using namespace cxxopts;

using namespace fcli;
using namespace fcli::literals;

Apm::Apm(): m_opts("apm", "Android Project Manager") {
  m_opts.add_options()
      ("colors", "Change number of colors in a palette (0, 8 or 256)",
          value<unsigned short>(), "NUM")
      ("choose-theme", "Change default theme")
      ("h,help", "Print the help message")
      ("version", "Print the versions information");
}

auto Apm::run(int& t_argc, char** t_argv) -> int {
  using namespace string_literals;

  const optional term_colors{Terminal().find_out_supported_colors()};
  if (term_colors) {
    Terminal::cache_colors_support(*term_colors);
  }

  try {
    m_config = make_unique<Config>();
  } catch (const exception& e) {
    cerr << Text::format_message(Text::Message::ERROR,
            "Couldn't load configuration: "s + e.what()) << endl;
    return EXIT_FAILURE;
  }

  // Using a pointer, since ParseResult doesn't have default constructor.
  unique_ptr<const ParseResult> parse_result;
  try {
    parse_result = make_unique<const ParseResult>(m_opts.parse(t_argc, t_argv));
  } catch (const option_not_exists_exception& e) {
    cerr << Text::format_message(Text::Message::ERROR, e.what()) << endl;
    cout << "Use <b>--help<r> to get available options"_fmt << endl;
    return EXIT_FAILURE;
  } catch (const OptionException& e) {
    cerr << Text::format_message(Text::Message::ERROR,
            "Invalid arguments syntax. "s + e.what()) << endl;
    return EXIT_FAILURE;
  }

  if (parse_result->arguments().empty() || parse_result->count("help") != 0U) {
    cout << m_opts.help() << flush;
    return EXIT_SUCCESS;
  }

  if (parse_result->count("colors") != 0U) {
    const auto colors{(*parse_result)["colors"].as<unsigned short>()};
    switch (colors) {
      case 0U: {
        Terminal::uncache_colors_support();
        break;
      }
      case 8U: {
        Terminal::cache_colors_support(Terminal::ColorsSupport::HAS_8_COLORS);
        break;
      }
      case 256U: {
        Terminal::cache_colors_support(Terminal::ColorsSupport::HAS_256_COLORS);
        break;
      }
      default: {
        cerr << "Wrong number of colors!"_err << endl;
        cout << "Possible values: "
                "<b>0<r> to completely disable text styling, "
                "<b>8<r> to use terminal's palette and <b>256<r>"_note << endl;
        return EXIT_FAILURE;
      }
    }
  }

  if (parse_result->count("version") != 0U) {
    print_versions();
    return EXIT_SUCCESS;
  }

  if (parse_result->count("choose-theme") != 0U) {
    request_theme();
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}

void Apm::request_theme() {
  using theme_t = pair<Theme::Name, string>;
  // Using array of pairs to keep items order.
  const array<theme_t, 4U> themes{{
    {Theme::Name::DEFAULT, "Terminal's palette"},
    {Theme::Name::MATERIAL_DARK, "Material Dark"},
    {Theme::Name::MATERIAL_LIGHT, "Material Light"},
    {Theme::Name::ARCTIC_DARK, "Arctic Dark"}
  }};

  const optional preferred_theme
                 {m_config->get<Theme::Name>(Config::Key::THEME)};
  const optional term_colors{Terminal::get_cached_colors_support()};

  optional<Theme::Name> current_theme;
  if (term_colors && preferred_theme) {
    if (term_colors == Terminal::ColorsSupport::HAS_256_COLORS) {
      current_theme = preferred_theme;
    } else {
      current_theme = Theme::Name::DEFAULT;
    }
  }

  cout << "Choose a theme (enter <b>0<r> to abort):"_fmt << endl;
  for (size_t i{0U}; i != themes.size(); ++i) {
    const auto& theme{themes.at(i)};

    cout << "  " + to_string(i + 1U) + ". " + theme.second << flush;
    if (current_theme && current_theme == theme.first) {
      if (current_theme == preferred_theme) {
        cout << " ~d~(current)<r>"_fmt << flush;
      } else {
        cout << " ~d~(active)<r>"_fmt << flush;
      }
    } else if (preferred_theme && preferred_theme == theme.first) {
      cout << " ~d~(preferred)<r>"_fmt << flush;
    }
    cout << endl;
  }

  size_t num{};
  while (true) {
    cout << "number> <b>"_fmt << flush;
    cin >> num;
    cout << "<r>"_fmt << flush;

    if (!cin) {
      cerr << "Invalid input! Try again"_err << endl;
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
    } else if (num > themes.size()) {
      cerr << "Wrong choice! Try again"_err << endl;
    } else {
      break;
    }
  }

  if (num == 0U) {
    cout << "Aborted"_warn << endl;
  } else {
    const auto choosed_theme{themes.at(num - 1U).first};
    if (m_config->apply<Theme::Name>(Config::Key::THEME, choosed_theme)) {
      cout << "Preference is saved"_note << endl;
    } else {
      cerr << "Couldn't apply theme!"_err << endl;
    }
  }
}

// TODO: print API version of installed SDK.
void Apm::print_versions() const {
  cout << "APM version: ~b~" APM_VERSION "<r>"_fmt << endl;
}
