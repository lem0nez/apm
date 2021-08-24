/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <cpr/api.h>
#include <cpr/cprtypes.h>
#include <cpr/status_codes.h>
#include <fcli/text.hpp>

#include "enum_array.hpp"
#include "sdk.hpp"
#include "utils.hpp"

using namespace std;
using namespace filesystem;

using namespace fcli;
using namespace fcli::literals;

using namespace cpr;
using namespace libzippp;
using namespace pugi;

Sdk::Sdk() {
  auto env_dir{getenv("XDG_DATA_HOME")};
  if (env_dir != nullptr) {
    m_root_dir_path = path(env_dir) / ROOT_DIR_NAME;
  } else {
    env_dir = getenv("HOME");
    if (env_dir == nullptr) {
      throw runtime_error("HOME isn't set");
    }
    m_root_dir_path = path(env_dir) / ".local" / "share" / ROOT_DIR_NAME;
  }
}

// -------------------- +
// Installation process |
// -------------------- +

auto Sdk::install(const shared_ptr<Config> t_config, const Terminal& t_term,
                  const unsigned short t_installed_api) -> int {
  if (t_installed_api != 0U) {
    cout << Text::format_copy("You have already installed SDK with API <b>" +
            to_string(t_installed_api) + "<r>.\nDo you want to override it?") <<
            endl;
    if (!Utils::request_confirm(false)) {
      return EXIT_SUCCESS;
    }
  }

  const auto get_progress_width{[&t_term] {
    constexpr unsigned short
        MAX_WIDTH{60U},
        FALL_BACK_WIDTH{20U};
    return Utils::get_term_width(t_term, MAX_WIDTH, FALL_BACK_WIDTH);
  }};

  const auto manifest{download_manifest(get_progress_width())};
  if (!manifest.document_element()) {
    return EXIT_FAILURE;
  }

  const auto api{request_api(manifest)};
  if (api == 0U) {
    return EXIT_FAILURE;
  }
  const auto api_str{to_string(api)};

  cout << Text::format_copy(
          "Installing SDK (API <b>" + api_str + "<r>):") << endl;
  create_dirs();
  // Used to store downloaded archives.
  TmpFile tmp_file(ios::binary);
  auto& tmp_ofs{tmp_file.get_stream()};

  constexpr array api_dependent_install_funcs{
    &Sdk::install_tools, &Sdk::install_build_tools, &Sdk::install_framework
  };
  for (const auto f : api_dependent_install_funcs) {
    // Retrieve terminal width every time, since it can be changed by the user.
    if (!invoke(f, this, manifest, api_str, tmp_file, get_progress_width())) {
      return EXIT_FAILURE;
    }
    if (!tmp_ofs.is_open()) {
      tmp_ofs.open(tmp_file.get_path(), ios::binary);
    }
  }

  bool install_api_independent_files{true};
  if (t_installed_api != 0U) {
    // Request confirmation before updating of
    // API independent files if all of them exist.
    bool confirm_update{true};
    error_code fs_err;

    for (size_t f{}; f != size<File>(); ++f) {
      if (!exists(get_file_path(static_cast<File>(f)), fs_err) || fs_err) {
        confirm_update = false;
        break;
      }
    }

    if (confirm_update) {
      cout << "Do you want to update API independent files?" << endl;
      if (!Utils::request_confirm(true)) {
        install_api_independent_files = false;
      }
      if (install_api_independent_files) {
        cout << "Updating API independent files:" << endl;
      }
    }
  }

  if (install_api_independent_files) {
    if (!install_tzdata(manifest, tmp_file, get_progress_width())) {
      return EXIT_FAILURE;
    }
    if (!install_assets(manifest, get_progress_width())) {
      return EXIT_FAILURE;
    }
  }

  if (!t_config->apply<decltype(api)>(Config::Key::SDK, api)) {
    cerr << "Couldn't preserve API version"_err << endl;
    return EXIT_FAILURE;
  }

  cout << "SDK installed." << endl;
  if (t_installed_api == 0U) {
    cout << "Use <b>-c<r> (<b>--create<r>) option "
            "to create a new project"_note << endl;
  }
  return EXIT_SUCCESS;
}

auto Sdk::download_manifest(
    const unsigned short t_progress_width) -> xml_document {
  using namespace string_literals;

  Progress progress("Downloading manifest", false, t_progress_width);
  progress.show();
  const Url url(string(REPO_RAW_URL_PREFIX) + "manifest.xml");
  const auto response{Get(url)};

  if (!check_response(response,
      "Couldn't download the manifest file", progress, false)) {
    return {};
  }

  xml_document doc;
  const auto parse_result{doc.load_string(response.text.c_str())};

  if (parse_result.status != status_ok) {
    cerr << Text::format_message(Text::Message::ERROR,
            "Couldn't parse the manifest file. "s +
            parse_result.description()) << endl;
    return {};
  }
  return doc;
}

auto Sdk::request_api(const xml_document& t_manifest) -> unsigned short {
  const auto api_attrs{t_manifest.select_nodes("/manifest/tools/set/@api")};
  set<unsigned short> apis;

  for (const auto& a : api_attrs) {
    const auto val{a.attribute().as_uint()};
    if (val != 0U) {
      apis.emplace(val);
    }
  }

  if (apis.empty()) {
    cerr << "No available API versions found"_err << endl;
    return {};
  }
  // TODO: remove it.
  if (apis.size() == 1U) {
    return *apis.cbegin();
  }

  cout << "Choose API version:" << endl;
  size_t num{};
  for (const auto a : apis) {
    cout << "  " + to_string(++num) + ". API " + to_string(a) << endl;
  }

  unsigned short api{};
  while (true) {
    cout << "version> <b>"_fmt << flush;
    cin >> api;
    cout << "<r>"_fmt << flush;

    if (!Utils::check_cin()) {
      continue;
    }
    if (apis.count(api) == 0U) {
      cerr << "Wrong version! Try again"_err << endl;
    } else {
      break;
    }
  }
  return api;
}

void Sdk::create_dirs() const {
  const array paths{
    m_root_dir_path,
    m_root_dir_path / TOOLS_SUBDIR_NAME,
    m_root_dir_path / JARS_SUBDIR_NAME
  };

  for (const auto& p : paths) {
    create_directories(p);
  }
}

// ------------------- +
// API dependent files |
// ------------------- +

auto Sdk::install_tools(const xml_document& t_manifest,
    const string& t_api, TmpFile<ofstream>& t_tmp_file,
    const unsigned short t_progress_width) const -> bool {
  Progress progress("Preparing to download tools", false, t_progress_width);
  progress.show();

  const string arch(Utils::get_arch_name(Utils::get_arch()));
  const xpath_query arch_query(("/manifest/tools/set[@api='" +
      t_api + "']/zip[text()='" + arch + "']").c_str());
  const auto arch_xpath_node{t_manifest.select_node(arch_query)};

  if (!arch_xpath_node) {
    progress.finish(false, Text::format_copy(
        "Tools aren't available for architecture <u>" + arch +
        "<r>. Try another API version, if available"));
    return false;
  }

  const auto checksum{get_sha256(arch_xpath_node.node())};
  if (checksum.empty()) {
    progress.finish(false, Text::format_copy(
        "Checksum of tools doesn't exist for architecture <u>" + arch + "<r>"));
    return false;
  }

  progress = "Downloading tools";
  auto& tmp_ofs{t_tmp_file.get_stream()};
  const Url url(string(REPO_RAW_URL_PREFIX) +
                "tools/api-" + t_api + '/' + arch + ".zip");
  const auto response{Utils::download(tmp_ofs, url, progress)};
  tmp_ofs.close();

  const auto& tmp_file_path{t_tmp_file.get_path()};
  if (!check_response_and_sha256(response,
      tmp_file_path, checksum, "tools", progress)) {
    return false;
  }

  ZipArchive zip(tmp_file_path);
  if (!zip.open()) {
    progress.finish(false, "Couldn't open archive with tools");
    return false;
  }

  error_code fs_err;
  for (const auto& e : zip.getEntries()) {
    const auto& name{e.getName()};
    const auto output_path{m_root_dir_path / TOOLS_SUBDIR_NAME / name};
    if (!extract_zip_entry(e, output_path, name, progress)) {
      return false;
    }

    permissions(output_path,
        perms::owner_exec | perms::group_exec | perms::others_exec,
        perm_options::add, fs_err);
    if (fs_err) {
      progress.finish(false, "Couldn't set permissions for " +
                      name + " (" + fs_err.message() + ')');
      return false;
    }
  }

  progress.finish(true,
      Text::format_copy("Tools for <u>" + arch + "<r> installed"));
  return true;
}

auto Sdk::install_build_tools(const xml_document& t_manifest,
    const string& t_api, TmpFile<ofstream>& t_tmp_file,
    const unsigned short t_progress_width) const -> bool {
  Progress progress("Preparing to download build tools",
                    false, t_progress_width);
  progress.show();

  const auto xpath_node{t_manifest.select_node(
      ("/manifest/build-tools/zip[@api='" + t_api + "']").c_str())};
  if (!xpath_node) {
    progress.finish(false, "Couldn't find build tools in the manifest");
    return false;
  }
  const auto node{xpath_node.node()};

  const auto url_attr{node.attribute("url")};
  if (!url_attr) {
    progress.finish(false, "URL to build tools doesn't exist");
    return false;
  }
  const Url url(url_attr.as_string());

  const auto checksum{get_sha256(node)};
  if (checksum.empty()) {
    progress.finish(false, "Checksum of build tools doesn't exist");
    return false;
  }

  progress.set_determined(true);
  progress = "Downloading build tools";
  auto& tmp_ofs{t_tmp_file.get_stream()};
  const auto response{Utils::download(tmp_ofs, url, progress)};
  tmp_ofs.close();

  const auto& tmp_file_path{t_tmp_file.get_path()};
  if (!check_response_and_sha256(response,
      tmp_file_path, checksum, "build tools", progress)) {
    return false;
  }
  // Checksum checker set progress as undetermined.

  // Notify about opening, since an archive is large.
  progress = "Opening archive with build tools";
  ZipArchive zip(tmp_file_path);
  if (!zip.open()) {
    progress.finish(false, "Couldn't open archive with build tools");
    return false;
  }

  for (auto t{node.first_child()}; t != nullptr; t = t.next_sibling()) {
    const path tool_path(t.text().get());
    if (tool_path.empty()) {
      progress.finish(false,
          "Couldn't extract build tools: no path provided for a tool");
      return false;
    }

    const auto entry{zip.getEntry(tool_path)};
    if (entry.isNull()) {
      progress.finish(false,
          "Couldn't extract build tools: failed to get ZIP entry \"" +
          tool_path.string() + '"');
      return false;
    }

    const string name(tool_path.filename());
    const auto output_path{m_root_dir_path / JARS_SUBDIR_NAME / name};
    if (!extract_zip_entry(entry, output_path, name, progress)) {
      return false;
    }
  }

  progress.finish(true, "Build tools installed");
  return true;
}

auto Sdk::install_framework(const xml_document& t_manifest,
    const string& t_api, TmpFile<ofstream>& t_tmp_file,
    const unsigned short t_progress_width) const -> bool {
  Progress progress("Preparing to download platform", false, t_progress_width);
  progress.show();

  const auto xpath_node{t_manifest.select_node(
      ("/manifest/platforms/zip[@api='" + t_api + "']").c_str())};
  if (!xpath_node) {
    progress.finish(false, "Couldn't find platform in the manifest");
    return false;
  }
  const auto node{xpath_node.node()};

  const auto url_attr{node.attribute("url")};
  if (!url_attr) {
    progress.finish(false, "URL to platform doesn't exist");
    return false;
  }
  const Url url(url_attr.as_string());

  const auto checksum{get_sha256(node)};
  if (checksum.empty()) {
    progress.finish(false, "Checksum of platform doesn't exist");
    return false;
  }

  const auto framework_node{node.child("framework")};
  if (!framework_node) {
    progress.finish(false, "Android framework path doesn't exist");
    return false;
  }
  const path framework_path(framework_node.text().get());

  progress.set_determined(true);
  progress = "Downloading platform";
  auto& tmp_ofs{t_tmp_file.get_stream()};
  const auto response{Utils::download(tmp_ofs, url, progress)};
  tmp_ofs.close();

  const auto& tmp_file_path{t_tmp_file.get_path()};
  if (!check_response_and_sha256(response,
      tmp_file_path, checksum, "platform", progress)) {
    return false;
  }

  progress = "Opening archive with Android framework";
  ZipArchive zip(tmp_file_path);
  if (!zip.open()) {
    progress.finish(false, "Couldn't open archive with Android framework");
    return false;
  }

  const auto entry{zip.getEntry(framework_path)};
  if (entry.isNull()) {
    progress.finish(false,
        "Couldn't extract Android framework: failed to get ZIP entry \"" +
        framework_path.string() + '"');
    return false;
  }

  if (!extract_zip_entry(entry, get_jar_path(Jar::FRAMEWORK),
      framework_path.filename().string(), progress)) {
    return false;
  }

  progress.finish(true, "Android framework installed");
  return true;
}

// --------------------- +
// API independent files |
// --------------------- +

auto Sdk::install_tzdata(
    const xml_document& t_manifest, TmpFile<ofstream>& t_tmp_file,
    const unsigned short t_progress_width) const -> bool {
  Progress progress("Preparing to download time zone database",
                    false, t_progress_width);
  progress.show();

  const auto xpath_node
      {t_manifest.select_node("/manifest/tzdata/zip[@latest='true']")};
  if (!xpath_node) {
    progress.finish(false, "Couldn't find the latest version of "
                           "time zone database in the manifest");
    return false;
  }

  const string version(xpath_node.node().text().get());
  const Url url(string(REPO_RAW_URL_PREFIX) + "tzdata/" + version + ".zip");

  const auto checksum{get_sha256(xpath_node.node())};
  if (checksum.empty()) {
    progress.finish(false, "Checksum of time zone database doesn't exist");
    return false;
  }

  progress = "Downloading time zone database";
  auto& tmp_ofs{t_tmp_file.get_stream()};
  const auto response{Utils::download(tmp_ofs, url, progress)};
  tmp_ofs.close();

  const auto& tmp_file_path{t_tmp_file.get_path()};
  if (!check_response_and_sha256(response,
      tmp_file_path, checksum, "time zone database", progress)) {
    return false;
  }

  ZipArchive zip(tmp_file_path);
  if (!zip.open()) {
    progress.finish(false, "Couldn't open archive with time zone database");
    return false;
  }

  const auto output_path{get_file_path(File::TZDATA)};
  const string name(output_path.filename());
  const auto entry{zip.getEntry(name)};

  if (entry.isNull()) {
    progress.finish(false, "Couldn't install time zone database: "
                    "failed to get ZIP entry \"" + name + '"');
    return false;
  }
  if (!extract_zip_entry(entry, output_path, name, progress)) {
    return false;
  }

  progress.finish(true,
      Text::format_copy("Time zone database <u>" + version + "<r> installed"));
  return true;
}

auto Sdk::install_assets(const xml_document& t_manifest,
    const unsigned short t_progress_width) const -> bool {
  Progress progress("Preparing to download assets", false, t_progress_width);
  progress.show();

  const auto xpath_nodes{t_manifest.select_nodes("/manifest/assets/file")};
  if (xpath_nodes.empty()) {
    progress.finish(false, "No assets found in the manifest");
    return false;
  }

  const EnumArray<File, path> output_paths{
    get_file_path(File::DEBUG_KEYSTORE),
    get_file_path(File::PROJECT_TEMPLATE)
    // TZDATA doesn't required.
  };

  // Pair of output path and friendly asset name.
  using asset_t = pair<const path&, string>;
  // Key is a file name.
  const map<string, asset_t> assets{
    {output_paths.get(File::DEBUG_KEYSTORE).filename(),
        {output_paths.get(File::DEBUG_KEYSTORE), "Debug keystore"}},
    {output_paths.get(File::PROJECT_TEMPLATE).filename(),
        {output_paths.get(File::PROJECT_TEMPLATE), "Project template"}}
  };

  for (const auto& n : xpath_nodes) {
    const auto node{n.node()};
    // Progress hides by the finish function at the end of the loop.
    progress.show();

    const string filename(node.text().get());
    if (filename.empty()) {
      progress.finish(false, "An asset doesn't have a name");
      return false;
    }

    const auto checksum{get_sha256(node)};
    if (checksum.empty()) {
      progress.finish(false, "Asset " + filename + " doesn't have checksum");
      return false;
    }

    const auto& asset{assets.at(filename)};
    const auto& output_path{asset.first};

    ofstream ofs(output_path, ios::binary);
    if (!ofs) {
      progress.finish(false, "Couldn't install asset " + filename +
          ": failed to open output file \"" + output_path.string() + '"');
      return false;
    }

    progress = "Downloading " + filename;
    const Url url(string(REPO_RAW_URL_PREFIX) + "assets/" + filename);
    const auto response{Download(ofs, url)};
    ofs.close();

    if (!check_response_and_sha256(response,
        output_path, checksum, "asset " + filename, progress)) {
      error_code fs_err;
      // Ignore any file system errors as it's already failed state.
      remove(output_path, fs_err);
      return false;
    }

    progress.finish(true, asset.second + " downloaded");
  }
  return true;
}

// ---------------- +
// Helper functions |
// ---------------- +

auto Sdk::check_response(const Response& t_response,
    const string_view t_failure_msg, Progress& t_progress,
    const bool t_fail_using_progress) -> bool {
  if (t_response.status_code == status::HTTP_OK) {
    return true;
  }

  if (!t_fail_using_progress) {
    t_progress.hide();
  }

  if (t_response.status_code == 0) {
    const auto msg{string(t_failure_msg) + ". " + t_response.error.message};

    if (t_fail_using_progress) {
      t_progress.finish(false, msg);
    } else {
      cerr << Text::format_message(Text::Message::ERROR, msg) << endl;
    }
  } else {
    auto msg{string(t_failure_msg) + " (status code <b>" +
             to_string(t_response.status_code) + "<r>)"};

    if (t_fail_using_progress) {
      Text::format(msg);
      t_progress.finish(false, msg);
    } else {
      cerr << Text::format_message(Text::Message::ERROR, msg) << endl;
    }
  }
  return false;
}

auto Sdk::get_sha256(const xml_node& t_node) -> string {
  const auto attr{t_node.attribute("sha256")};
  if (!attr) {
    return {};
  }
  return attr.as_string();
}

auto Sdk::check_sha256(const path& t_file_path, const string_view t_checksum,
    Progress& t_progress, const string_view t_failure_msg) -> bool {
  t_progress.set_determined(false);
  t_progress = "Calculating checksum";

  if (Utils::calc_sha256(t_file_path) != t_checksum) {
    t_progress.finish(false,
        string(t_failure_msg) + ": invalid checksum. Try to set up SDK again");
    return false;
  }
  return true;
}

auto Sdk::check_response_and_sha256(const Response& t_response,
    const path& t_file_path, const string_view t_checksum,
    const string_view t_subject, Progress& t_progress) -> bool {
  const string subject_str(t_subject);

  if (!check_response(t_response,
      "Couldn't download " + subject_str, t_progress)) {
    return false;
  }
  if (!check_sha256(t_file_path, t_checksum, t_progress,
      "Couldn't install " + subject_str)) {
    return false;
  }
  return true;
}

auto Sdk::extract_zip_entry(const ZipEntry& t_entry, const path& t_output_path,
    const string_view t_name, Progress& t_progress) -> bool {
  const string name_str(t_name);

  ofstream ofs(t_output_path, ios::binary);
  if (!ofs) {
    t_progress.finish(false, "Couldn't extract " + name_str +
        ": failed to open output file \"" + t_output_path.string() + '"');
    return false;
  }

  t_progress = "Extracting " + name_str;
  if (const auto status{t_entry.readContent(ofs)}; status != LIBZIPPP_OK) {
    t_progress.finish(false, Text::format_copy("Couldn't extract " + name_str +
        ". libzippp return code: <b>" + to_string(status) + "<r>"));
    return false;
  }
  return true;
}

// ------- +
// Getters |
// ------- +

auto Sdk::get_tool_path(const Tool t_tool) const -> path {
  const EnumArray<Tool, string> names{"aapt2", "zipalign"};
  return m_root_dir_path / TOOLS_SUBDIR_NAME / names.get(t_tool);
}

auto Sdk::get_jar_path(const Jar t_jar) const -> path {
  const EnumArray<Jar, string> names{"apksigner.jar", "d8.jar", "android.jar"};
  return m_root_dir_path / JARS_SUBDIR_NAME / names.get(t_jar);
}

auto Sdk::get_file_path(const File t_file) const -> path {
  const EnumArray<File, string> names{
    "debug.jks", "project-template.zip", "tzdata"
  };
  return m_root_dir_path / names.get(t_file);
}
