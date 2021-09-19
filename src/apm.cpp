/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <array>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <fcli/progress.hpp>
#include <fcli/text.hpp>
#include <fcli/theme.hpp>

#include "apm.hpp"
#include "project.hpp"
#include "utils.hpp"

using namespace std;
using namespace filesystem;
using namespace string_literals;

using namespace cxxopts;
using namespace fcli;
using namespace fcli::literals;

using ColorsSupport = Terminal::ColorsSupport;
using Message = Text::Message;

Apm::Apm(error_condition& t_err): m_opts("apm", "Android Project Manager") {
  // Project related options.
  m_opts.add_options()
      // This is an optional positional option.
      ("dir", "Set a project directory", value<path>())
      ("c,create", "Create a new project")
      ("b,build", "Build a project")
      ("t,type", "Change build type: debug (default) or release",
          value<string>(), "TYPE")
      ("o,output", "Set path of the output APK file", value<path>(), "FILE");

  // Options that don't require a project directory.
  m_opts.add_options("Other")
      ("s,set-up", "Download and install SDK")
      ("j,set-jks", "Set a Java KeyStore for signing the release APK files",
          value<path>(), "FILE")
      ("colors", "Change number of colors in a palette (0, 8 or 256)",
          value<unsigned short>(), "NUM")
      ("choose-theme", "Choose default theme")
      ("h,help", "Print the help message")
      ("version", "Print the versions information");

  m_opts.custom_help("OPTION...");
  m_opts.positional_help("[DIR]");
  m_opts.parse_positional("dir");

  const optional term_colors{m_term.find_out_supported_colors()};
  if (term_colors) {
    Terminal::cache_colors_support(*term_colors);
  }

  try {
    m_config = make_shared<Config>();
  } catch (const exception& e) {
    cerr << Text::format_message(Message::ERROR,
            "Couldn't load a configuration: "s + e.what()) << endl;
    t_err = {EXIT_FAILURE, generic_category()};
    return;
  }

  try {
    m_sdk = make_shared<Sdk>();
  } catch (const exception& e) {
    cerr << Text::format_message(Message::ERROR,
            "Couldn't prepare SDK: "s + e.what()) << endl;
    t_err = {EXIT_FAILURE, generic_category()};
    return;
  }
}

auto Apm::run(int& t_argc, char** t_argv) -> int {
  constexpr string_view SDK_NOT_INSTALLED_MSG(
      "SDK not installed. Use <b>-s<r> (<b>--set-up<r>) option to install it");

  // Using a pointer, since ParseResult doesn't have default constructor.
  unique_ptr<const ParseResult> parse_result;
  try {
    parse_result = make_unique<const ParseResult>(m_opts.parse(t_argc, t_argv));
  } catch (const option_not_exists_exception& e) {
    cerr << Text::format_message(Message::ERROR, e.what()) << endl;
    cout << "Use <b>--help<r> to get available options"_fmt << endl;
    return EXIT_FAILURE;
  } catch (const OptionException& e) {
    cerr << Text::format_message(Message::ERROR,
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

  // --------------- +
  // Project related |
  // --------------- +

  path project_dir;
  if (parse_result->count("dir") != 0U) {
    project_dir = (*parse_result)["dir"].as<path>();
  }

  // Iterate through options that depend on SDK and require a project directory.
  for (const auto& o : {"create", "build"}) {
    if (parse_result->count(o) == 0U) {
      continue;
    }

    if (!installed_sdk) {
      cerr << Text::format_message(Message::ERROR,
              SDK_NOT_INSTALLED_MSG) << endl;
      return EXIT_FAILURE;
    }
    if (project_dir.empty()) {
      cerr << "A project directory not specified"_err << endl;
      return EXIT_FAILURE;
    }
    break;
  }

  Project project;
  // Iterate through options that require an instance of existence project.
  for (const auto& o : {"build"}) {
    if (parse_result->count(o) == 0U) {
      continue;
    }

    try {
      /*
       * Show a progress because the Project constructor deals with file system
       * operations, performance of which depend on storage type and may be
       * slow.
       */
      constexpr string_view PROGRESS_TEXT{"Loading the project"};
      constexpr unsigned short
          MAX_PROGRESS_WIDTH{PROGRESS_TEXT.length() + 10U},
          FALL_BACK_PROGRESS_WIDTH{15U};
      Progress progress(PROGRESS_TEXT, false, Utils::get_term_width(
          m_term, MAX_PROGRESS_WIDTH, FALL_BACK_PROGRESS_WIDTH));
      progress.show();

      project = Project(project_dir);
    } catch (const exception& e) {
      cerr << Text::format_message(Message::ERROR,
              "Couldn't load the project: "s + e.what()) << endl;
      return EXIT_FAILURE;
    }
    break;
  }

  if (parse_result->count("create") != 0U) {
    try {
      return Project::create(project_dir, *installed_sdk,
             m_sdk->get_file_path(Sdk::File::PROJECT_TEMPLATE), m_term);
    } catch (const exception& e) {
      cerr << Text::format_message(Message::ERROR,
              "Couldn't create a new project: "s + e.what()) << endl;
      return EXIT_FAILURE;
    }
  }

  if (parse_result->count("build") != 0U) {
    bool is_debug_build{true};
    if (parse_result->count("type") != 0U) {
      const auto type{(*parse_result)["type"].as<string>()};

      if (type.empty()) {
        cerr << "Build type must not be empty"_err << endl;
        return EXIT_FAILURE;
      }

      const auto type_len{type.length()};
      // Allow first N characters of build type.
      if ("release"s.substr(0U, type_len) == type) {
        is_debug_build = false;
      } else if ("debug"s.substr(0U, type_len) != type) {
        cerr << "Build type must be either "
                "<u>d~d~ebug<r> or <u>r~d~elease<r>"_err << endl;
        return EXIT_FAILURE;
      }
    }

    path output_apk;
    if (parse_result->count("output") != 0U) {
      output_apk = (*parse_result)["output"].as<path>();
      // Path checks will be performed by the build function.
    }

    try {
      return project.build(*this, is_debug_build, output_apk);
    } catch (const exception& e) {
      cerr << Text::format_message(Message::ERROR,
              "Couldn't build the project: "s + e.what()) << endl;
      return EXIT_FAILURE;
    }
  }

  // ------------- +
  // Other options |
  // ------------- +

  if (parse_result->count("set-up") != 0U) {
    try {
      return m_sdk->install(m_config, m_term,
                            installed_sdk ? *installed_sdk : 0U);
    } catch (const exception& e) {
      cerr << Text::format_message(Message::ERROR,
              "Couldn't set up SDK: "s + e.what()) << endl;
      return EXIT_FAILURE;
    }
  }

  if (parse_result->count("set-jks") != 0U) {
    try {
      set_release_jks((*parse_result)["set-jks"].as<path>());
    } catch (const exception& e) {
      // Don't save, probably, incomplete configuration.
      m_config->unbind_file();
      cerr << Text::format_message(Message::ERROR,
              "Couldn't set the Java KeyStore: "s + e.what()) << endl;
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
    cout << Text::format_message(Message::WARNING,
            SDK_NOT_INSTALLED_MSG) << endl;
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
      Terminal::cache_colors_support(ColorsSupport::HAS_8_COLORS);
      break;
    }
    case 256U: {
      Terminal::cache_colors_support(ColorsSupport::HAS_256_COLORS);
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

void Apm::set_release_jks(const path& t_path) const {
  if (!exists(t_path)) {
    throw runtime_error("file doesn't exist");
  }
  if (is_directory(t_path)) {
    throw runtime_error("it's must be a file");
  }

  cout << "Enter the <u>alias name<r> of a private key.\n"
          "If the keystore contains only one key, you can skip it."_fmt << endl;
  string alias;
  do {
    cout << "alias> ~c~"_fmt << flush;
    getline(cin, alias);
    cout << "<r>"_fmt << flush;
  } while (!Utils::check_cin());

  cout << "Does the private key <u>have a password<r>?"_fmt << endl;
  const auto key_has_passwd{Utils::request_confirm()};

  const array<function<bool()>, 3U> apply_funcs{
    [&] {
      return m_config->apply<path>(
             Config::Key::JKS_PATH, absolute(t_path), false);
    },
    [&] {
      if (alias.empty()) {
        return m_config->remove(Config::Key::JKS_KEY_ALIAS, false);
      }
      return m_config->apply<string>(Config::Key::JKS_KEY_ALIAS, alias, false);
    },
    [&] {
      return m_config->apply<bool>(
             Config::Key::JKS_KEY_HAS_PASSWD, key_has_passwd, false);
    }
  };

  for (const auto& f : apply_funcs) {
    if (!f()) {
      throw runtime_error("failed to apply configuration");
    }
  }
  if (!m_config->save()) {
    throw runtime_error("failed to save configuration");
  }
  cout << "Configuration is saved"_note << endl;
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
    if (term_colors == ColorsSupport::HAS_256_COLORS) {
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
    cin >> num;
    cout << "<r>"_fmt << flush;

    if (!Utils::check_cin()) {
      continue;
    }
    if (num > themes.size()) {
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
