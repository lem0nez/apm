/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <fcli/progress.hpp>
#include <fcli/text.hpp>

#include <libzippp/libzippp.h>
#include <pugixml.hpp>

#include "enum_array.hpp"
#include "project.hpp"
#include "utils.hpp"

using namespace std;
using namespace filesystem;

using namespace fcli;
using namespace fcli::literals;

// ---------------- +
// Project creation |
// ---------------- +

auto Project::create(const path& t_dir, const unsigned short t_sdk_api,
                     const path& t_templ_zip, const Terminal& t_term) -> int {
  using namespace string_literals;

  constexpr unsigned short
      MAX_PROGRESS_WIDTH{30U},
      FALL_BACK_PROGRESS_WIDTH{10U};
  error_code fs_err;

  if (exists(t_dir)) {
    const string type(is_directory(t_dir, fs_err) ? "Directory" : "File");
    cerr << Text::format_message(Text::Message::ERROR,
            type + " \"" + t_dir.string() + "\" already exists. "
            "You must specify a directory that doesn't exist") << endl;
    return EXIT_FAILURE;
  }

  const auto
      app_name{request_app_name()},
      package{request_package()};
  cout << Text::format_copy("Minimum SDK version: <b>" +
          to_string(t_sdk_api) + "<r>") << endl;
  const auto min_api{request_min_api(t_sdk_api)};

  cout << "\nCreating the project:" << endl;
  Progress progress("Extracting a template", false, Utils::get_term_width(
                    t_term, MAX_PROGRESS_WIDTH, FALL_BACK_PROGRESS_WIDTH));
  progress.show();

  try {
    extract_template(t_dir, t_templ_zip);
  } catch (const exception& e) {
    progress.finish(false, "Couldn't extract a template: "s + e.what());
    remove_all(t_dir, fs_err);
    return EXIT_FAILURE;
  }
  progress.finish(true, "Template extracted");

  progress = "Organizing resources";
  progress.show();
  try {
    organize_resources(directory_entry(t_dir / "app" / "res"), min_api);
  } catch (const exception& e) {
    progress.finish(false, "Couldn't organize resources: "s + e.what());
    remove_all(t_dir, fs_err);
    return EXIT_FAILURE;
  }
  progress.finish(true, "Resources organized");

  progress = "Configuring the project";
  progress.show();
  try {
    expand_variables(directory_entry(t_dir), package, t_sdk_api, min_api);
    set_app_name(t_dir / "app" / "res" / "values" / "strings.xml", app_name);
    relocate_class(
        directory_entry(t_dir / "app" / "java"), "MainActivity.java", package);
  } catch (const exception& e) {
    progress.finish(false, "Couldn't configure the project: "s + e.what());
    remove_all(t_dir, fs_err);
    return EXIT_FAILURE;
  }
  progress.finish(true, "Configuration is done");

  cout << "Project created." << endl;
  return EXIT_SUCCESS;
}

auto Project::request_app_name() -> string {
  const string
      prompt_msg("Application name: "),
      input_color{"~c~"_fmt};
  string name;

  cout << prompt_msg << input_color << flush;
  while (true) {
    // Allow whitespace characters using getline.
    getline(cin, name);
    cout << "<r>"_fmt << flush;

    if (!Utils::check_cin()) {
      cout << prompt_msg;
    } else if (!name.empty()) {
      break;
    }
    // Colorize input even if the user moved
    // to new line (entered the empty string).
    cout << input_color << flush;
  }
  return name;
}

auto Project::request_package() -> string {
  using namespace regex_constants;
  // Package name must contain at least two segments
  // and each segment must start with a letter.
  const regex pattern(R"([a-z]\w*(\.[a-z]\w*)+)", icase | nosubs | optimize);
  string package;

  while (true) {
    cout << "Package name: ~c~"_fmt << flush;
    cin >> package;
    cout << "<r>"_fmt << flush;

    if (!Utils::check_cin()) {
      continue;
    }
    // Match entire string.
    if (!regex_match(package, pattern)) {
      cerr << "Invalid package name! Try again"_err << endl;
    } else {
      break;
    }
  }
  return package;
}

auto Project::request_min_api(
    const unsigned short t_sdk_api) -> unsigned short {
  unsigned short level{};

  while (true) {
    cout << "Minimum API level: <b>"_fmt << flush;
    cin >> level;
    cout << "<r>"_fmt << flush;

    if (!Utils::check_cin()) {
      continue;
    }
    if (level < MIN_API) {
      cerr << Text::format_message(Text::Message::ERROR, "API level must be "
              "at least <b>" + to_string(MIN_API) + "<r>") << endl;
    } else if (level > t_sdk_api) {
      cerr << "API level must <b>not<r> be greater "
              "then minimum SDK version"_err << endl;
    } else {
      break;
    }
  }
  return level;
}

void Project::extract_template(const path& t_dest, const path& t_templ_zip) {
  create_directories(t_dest);

  libzippp::ZipArchive archive(t_templ_zip);
  if (!archive.open()) {
    throw runtime_error(
          "failed to open archive \"" + t_templ_zip.string() + '"');
  }

  for (const auto& e : archive.getEntries()) {
    const auto& name{e.getName()};
    const auto output_path{t_dest / name};

    if (e.isDirectory()) {
      create_directory(output_path);
      continue;
    }

    ofstream ofs(output_path, ios::binary);
    if (!ofs) {
      throw runtime_error(
            "failed to open output file \"" + output_path.string() + '"');
    }
    if (const auto status{e.readContent(ofs)}; status != LIBZIPPP_OK) {
      throw runtime_error("failed to extract \"" + name +
            "\" (libzippp return code is " + to_string(status) + '"');
    }
  }
}

void Project::organize_resources(const directory_entry& t_res_dir,
                                 const unsigned short t_min_api) {
  constexpr unsigned short
      MOVE_DRAWABLE_API{24U},
      KEEP_MIPMAP_ANYDPI_API{26U};
  const auto& res_path{t_res_dir.path()};

  if (t_min_api >= MOVE_DRAWABLE_API) {
    // Move files from “drawable-v<MOVE_DRAWABLE_API>”
    // to “drawable” and delete the source directory.

    const auto src_dir
        {res_path / ("drawable-v" + to_string(MOVE_DRAWABLE_API))};
    for (const auto& f : directory_iterator(src_dir)) {
      rename(f, res_path / "drawable" / f.path().filename());
    }
    remove(src_dir);
  }

  if (t_min_api >= KEEP_MIPMAP_ANYDPI_API) {
    // Rename “mipmap-anydpi-v<KEEP_MIPMAP_ANYDPI_API>”
    // and keep only this mipmap folder.

    const auto anydpi_dir
        {res_path / ("mipmap-anydpi-v" + to_string(KEEP_MIPMAP_ANYDPI_API))};
    rename(anydpi_dir, res_path / "mipmap-anydpi");

    constexpr string_view MIPMAP_DIR_PREFIX("mipmap");
    for (const auto& d : directory_iterator(t_res_dir)) {
      const auto name{d.path().filename().string()};

      if (name.substr(0U, MIPMAP_DIR_PREFIX.size()) == MIPMAP_DIR_PREFIX &&
          name != "mipmap-anydpi") {
        remove_all(d);
      }
    }
  }
}

void Project::expand_variables(const directory_entry& t_project_root,
    const string_view t_package, const unsigned short t_sdk_api,
    const unsigned short t_min_api) {
  constexpr string_view
      VAR_PREFIX("{{"),
      VAR_POSTFIX("}}");

  enum class Var {
    PACKAGE,
    MIN_API_LEVEL,
    MIN_SDK_VERSION,
    _COUNT
  };

  // Pair of variable name and value.
  using var_t = pair<string, string>;
  const EnumArray<Var, var_t> vars{
    var_t("PACKAGE", string(t_package)),
    var_t("MIN_API_LEVEL", to_string(t_min_api)),
    var_t("MIN_SDK_VERSION", to_string(t_sdk_api))
  };

  // Key is a file path (relative to the root project directory)
  // and value is variables that this file contains.
  const map<path, set<Var>> files{
    {"apm.xml", {Var::MIN_SDK_VERSION, Var::MIN_API_LEVEL}},
    {path("app") / "AndroidManifest.xml", {Var::PACKAGE}},
    {path("app") / "java" / "MainActivity.java", {Var::PACKAGE}}
  };

  for (const auto& [file, file_vars] : files) {
    const auto file_path{t_project_root / file};
    ifstream ifs(file_path);
    if (!ifs) {
      throw runtime_error(
            "failed to open input file \"" + file_path.string() + '"');
    }
    string content{istreambuf_iterator(ifs), istreambuf_iterator<char>()};
    ifs.close();

    for (const auto v : file_vars) {
      const auto var
          {string(VAR_PREFIX) + vars.get(v).first + string(VAR_POSTFIX)};
      const auto& val{vars.get(v).second};

      if (const auto pos{content.find(var)}; pos != string::npos) {
        content.replace(pos, var.length(), val);
      }
    }

    ofstream ofs(file_path);
    if (!ofs) {
      throw runtime_error(
            "failed to open output file \"" + file_path.string() + '"');
    }
    ofs << content << flush;
  }
}

void Project::set_app_name(const path& t_strings_xml,
                           const string_view t_name) {
  using namespace pugi;

  // Using pugixml instead of simple string replacing, since the name can
  // contain symbols that must be escaped / replaced to be used in a XML file.
  xml_document strings;
  const auto parse_result{strings.load_file(t_strings_xml.c_str())};
  if (!parse_result) {
    throw runtime_error("failed to parse XML file \"" + t_strings_xml.string() +
                        "\" (" + parse_result.description() + ')');
  }

  auto name_xpath_node
      {strings.select_node("/resources/string[@name='app_name']")};
  if (!name_xpath_node) {
    throw runtime_error(R"(failed to find the "app_name" string in ")" +
                        t_strings_xml.string() + '"');
  }
  if (!name_xpath_node.node().text().set(string(t_name).c_str())) {
    throw runtime_error("failed to set the application name");
  }

  const string indent(4U, ' ');
  // Preserve the encoding attribute in XML declaration.
  auto decl{strings.prepend_child(node_declaration)};
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";

  if (!strings.save_file(t_strings_xml.c_str(), indent.c_str(),
                         format_default, encoding_utf8)) {
    throw runtime_error(
          "failed to save XML file \"" + t_strings_xml.string() + '"');
  }
}

void Project::relocate_class(const directory_entry& t_root_dir,
                             const path& t_file, const string_view t_package) {
  auto dest_path{t_root_dir.path()};
  dest_path += '/';
  for (const auto c : t_package) {
    dest_path += (c == '.') ? '/' : c;
  }

  create_directories(dest_path);
  rename(t_root_dir / t_file, dest_path / t_file);
}
