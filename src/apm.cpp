/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <array>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <utility>

#include <fcli/text.hpp>
#include <fcli/theme.hpp>

#include "apm.hpp"

using namespace std;
using namespace string_literals;

using namespace cxxopts;
using namespace fcli;
using namespace fcli::literals;

Apm::Apm(error_condition& t_err): m_opts("apm", "Android Project Manager") {
  m_opts.add_options()
      ("s,set-up", "Download and install SDK")
      ("colors", "Change number of colors in a palette (0, 8 or 256)",
          value<unsigned short>(), "NUM")
      ("choose-theme", "Choose default theme")
      ("h,help", "Print the help message")
      ("version", "Print the versions information");

  const optional term_colors{m_term.find_out_supported_colors()};
  if (term_colors) {
    Terminal::cache_colors_support(*term_colors);
  }

  try {
    m_config = make_shared<Config>();
  } catch (const exception& e) {
    cerr << Text::format_message(Text::Message::ERROR,
            "Couldn't load configuration: "s + e.what()) << endl;
    t_err = {EXIT_FAILURE, generic_category()};
    return;
  }

  try {
    m_sdk = make_unique<Sdk>();
  } catch (const exception& e) {
    cerr << Text::format_message(Text::Message::ERROR,
            "Couldn't prepare SDK: "s + e.what()) << endl;
    t_err = {EXIT_FAILURE, generic_category()};
    return;
  }
}

auto Apm::run(int& t_argc, char** t_argv) -> int {
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

  if (parse_result->count("colors") != 0U) {
    const auto num{(*parse_result)["colors"].as<unsigned short>()};
    if (!set_colors(num)) {
      return EXIT_FAILURE;
    }
  }

  const optional installed_sdk{m_config->get<unsigned short>(Config::Key::SDK)};
  if (parse_result->count("set-up") != 0U) {
    try {
      return m_sdk->install(m_config, m_term,
                            installed_sdk ? *installed_sdk : 0U);
    } catch (const exception& e) {
      cerr << Text::format_message(Text::Message::ERROR,
              "Couldn't set up SDK: "s + e.what()) << endl;
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  if (parse_result->count("choose-theme") != 0U) {
    request_theme();
    return EXIT_SUCCESS;
  }

  if (parse_result->count("version") != 0U) {
    print_versions();
    return EXIT_SUCCESS;
  }

  cout << m_opts.help() << flush;
  if (parse_result->count("help") == 0U && !installed_sdk) {
    cout << "SDK not installed. "
            "Use <b>-s<r> (<b>--set-up<r>) option to install it"_warn << endl;
  }
  return EXIT_SUCCESS;
}

auto Apm::set_colors(const unsigned short t_num) -> bool {
  switch (t_num) {
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
      cerr << "Wrong number of colors! Possible values: "
              "<b>0<r> to completely disable text styling, "
              "<b>8<r> to use terminal's palette and <b>256<r>"_err << endl;
      return false;
    }
  }
  return true;
}

void Apm::request_theme(istream& t_istream) {
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
  for (size_t i{}; i != themes.size(); ++i) {
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
    t_istream >> num;
    cout << "<r>"_fmt << flush;

    if (!t_istream) {
      cerr << "Invalid input! Try again"_err << endl;
      t_istream.clear();
      t_istream.ignore(numeric_limits<streamsize>::max(), '\n');
    } else if (num > themes.size()) {
      cerr << "Wrong choice! Try again"_err << endl;
    } else {
      break;
    }
  }

  if (num == 0U) {
    return;
  }

  const auto chosen_theme{themes.at(num - 1U).first};
  if (m_config->apply<Theme::Name>(Config::Key::THEME, chosen_theme)) {
    cout << "Preference is saved"_note << endl;
  } else {
    cerr << "Couldn't apply theme"_err << endl;
  }
}

void Apm::print_versions() const {
  cout << "APM version: <b>" APM_VERSION "<r>"_fmt << endl;

  const optional sdk_api{m_config->get<unsigned short>(Config::Key::SDK)};
  if (sdk_api) {
    cout << Text::format_copy(
            "API of SDK: <b>" + to_string(*sdk_api) + "<r>") << endl;
  }
}