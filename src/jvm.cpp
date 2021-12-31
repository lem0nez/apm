/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#include <algorithm>
#include <array>
#include <sys/sysinfo.h>

#include "jvm.hpp"

using namespace std;
using namespace jni;

using namespace jvm_tools;
using Jar = Sdk::Jar;

Jvm::Jvm(const flags_t t_init_tools, const shared_ptr<const Sdk> t_sdk) {
  const auto heap_opts{get_heap_opts()};
  string classpath{"-Djava.class.path=" +
                   t_sdk->get_jar_path(Jar::APM_JNI).string()};
  if ((t_init_tools & D8) != flags_t{}) {
    classpath += ':' + t_sdk->get_jar_path(Jar::D8).string();
  }
  if ((t_init_tools & APKSIGNER) != flags_t{}) {
    classpath += ':' + t_sdk->get_jar_path(Jar::APKSIGNER).string();
  }

  array str_opts{heap_opts.first, heap_opts.second, classpath};
  array<JavaVMOption, str_opts.size()> opts{};
  for (size_t o{}; o != opts.size(); ++o) {
    opts.at(o).optionString = str_opts.at(o).data();
  }

  JavaVMInitArgs args;
  args.version = JNI_VERSION;
  args.ignoreUnrecognized = jni_false;
  args.nOptions = opts.size();
  args.options = opts.data();

  void* env{};
  if (JNI_CreateJavaVM(&m_vm, &env, static_cast<void*>(&args)) != jni_ok) {
    throw runtime_error("failed to create a VM");
  }
  m_env = static_cast<JNIEnv*>(env);

  safe_java_exec<void>(
      [this] { redirect_output(); }, "failed to redirect standard output");
  safe_java_exec<void>(
      [this] { set_security_manager(); }, "failed to set SecurityManager");

  if ((t_init_tools & JAVAC) != flags_t{}) {
    safe_java_exec<void>(
        [this] { init_javac(); }, "failed to initialize the Java compiler");
  }
  if ((t_init_tools & (D8 | APKSIGNER)) != flags_t{}) {
    safe_java_exec<void>([&] {
      this->init_android_tools(t_init_tools, t_sdk);
    }, "failed to initialize Android tools");
  }
}

void Jvm::redirect_output() {
  m_output = Class<Output>::Find(*m_env);

  const auto reset{m_output.GetStaticMethod<void()>(*m_env, "reset")};
  m_output_reset = make_unique<decltype(reset)>(reset);

  const auto
      get_out{m_output.GetStaticMethod<String()>(*m_env, "getOut")},
      get_err{m_output.GetStaticMethod<String()>(*m_env, "getErr")};
  m_output_get_out = make_unique<decltype(get_out)>(get_out);
  m_output_get_err = make_unique<decltype(get_err)>(get_err);

  const auto redirect{m_output.GetStaticMethod<void()>(*m_env, "redirect")};
  m_output.Call(*m_env, redirect);
}

void Jvm::set_security_manager() const {
  struct System {
    static constexpr auto Name() { return "java/lang/System"; }
  };
  struct SecurityManager {
    static constexpr auto Name() { return "java/lang/SecurityManager"; }
  };
  struct ApmSecurityManager {
    static constexpr auto Name()
        { return "com/github/lem0nez/apm/SecurityManager"; }
  };

  const auto system{Class<System>::Find(*m_env)};
  const auto sm{Class<SecurityManager>::Find(*m_env)};
  const auto apm_sm{Class<ApmSecurityManager>::Find(*m_env)};

  const auto set_sm{system.GetStaticMethod<void(Object<SecurityManager>)>(
      *m_env, "setSecurityManager")};
  const auto apm_sm_obj{apm_sm.New(*m_env, apm_sm.GetConstructor(*m_env))};
  system.Call(*m_env, set_sm, Cast(*m_env, sm, apm_sm_obj));
}

void Jvm::init_javac() {
  struct ToolProvider {
    static constexpr auto Name() { return "javax/tools/ToolProvider"; }
  };

  const auto tool_provider{Class<ToolProvider>::Find(*m_env)};
  const auto get_javac{tool_provider.GetStaticMethod<Object<JavaCompiler>()>(
                       *m_env, "getSystemJavaCompiler")};
  m_javac_obj = tool_provider.Call(*m_env, get_javac);

  const auto javac{Class<JavaCompiler>::Find(*m_env)};
  const auto javac_run{javac.GetMethod<javac_run_t>(*m_env, "run")};
  m_javac_run = make_unique<decltype(javac_run)>(javac_run);
}

void Jvm::init_android_tools(const flags_t t_tools,
                             const shared_ptr<const Sdk> t_sdk) {
  const auto tool{Class<Tool>::Find(*m_env)};
  // Constructor takes path of a JAR file that must
  // contain Manifest file with the “Main-Class” attribute.
  const auto tool_init{tool.GetConstructor<String>(*m_env)};

  const auto tool_run{tool.GetMethod<jint(Array<String>)>(*m_env, "run")};
  m_tool_run = make_unique<decltype(tool_run)>(tool_run);

  // Don't require file existence since the constructor already checked it.
  if ((t_tools & D8) != flags_t{}) {
    m_d8_obj = tool.New(*m_env, tool_init,
               Make<String>(*m_env, t_sdk->get_jar_path(Jar::D8, false)));
  }
  if ((t_tools & APKSIGNER) != flags_t{}) {
    m_apksigner_obj = tool.New(*m_env, tool_init,
        Make<String>(*m_env, t_sdk->get_jar_path(Jar::APKSIGNER, false)));
  }
}

Jvm::~Jvm() {
  // Classes and objects must be deleted BEFORE destroying of a VM.
  m_apksigner_obj.reset();
  m_d8_obj.reset();
  m_javac_obj.reset();
  m_output.reset();

  m_vm->DestroyJavaVM();
}

// ----- +
// Tools |
// ----- +

auto Jvm::javac(const vector<string>& t_args,
                string& t_out, string& t_err) const -> int {
  if (!m_javac_obj) {
    throw runtime_error("Java compiler isn't initialized");
  }

  // Don't initialize it since the standard output streams already redirected.
  const Local<Object<OutputStream>> os;
  reset_output();
  const auto result{safe_java_exec<jint>([&] {
    return m_javac_obj.Call(*m_env, *m_javac_run,
           Local<Object<InputStream>>(), os, os, make_args(t_args));
  }, "Java compiler thrown an exception")};
  get_output(t_out, t_err);
  return result;
}

auto Jvm::d8(const vector<string>& t_args,
             string& t_out, string& t_err) const -> int {
  if (!m_d8_obj) {
    throw runtime_error("d8 isn't initialized");
  }

  reset_output();
  const auto result{safe_java_exec<jint>([&] {
    return m_d8_obj.Call(*m_env, *m_tool_run, make_args(t_args));
  }, "d8 thrown an exception")};
  get_output(t_out, t_err);
  return result;
}

auto Jvm::apksigner(const vector<string>& t_args,
                    string& t_out, string& t_err) const -> int {
  if (!m_apksigner_obj) {
    throw runtime_error("apksigner isn't initialized");
  }

  reset_output();
  const auto result{safe_java_exec<jint>([&] {
    return m_apksigner_obj.Call(*m_env, *m_tool_run, make_args(t_args));
  }, "apksigner thrown an exception")};
  get_output(t_out, t_err);
  return result;
}

// ---------------- +
// Helper functions |
// ---------------- +

auto Jvm::make_args(const vector<string>& t_args) const ->
    Local<Array<String>> {
  const auto count{t_args.size()};
  auto args{Array<String>::New(*m_env, count)};

  // Convert std::string to jni::String.
  for (size_t a{}; a != count; ++a) {
    args.Set(*m_env, a, Make<String>(*m_env, t_args.at(a)));
  }
  return args;
}

void Jvm::reset_output() const {
  safe_java_exec<void>([this] {
    m_output.Call(*m_env, *m_output_reset);
  }, "failed to reset output");
}

void Jvm::get_output(string& t_out, string& t_err) const {
  safe_java_exec<void>([&] {
    t_out = Make<string>(*m_env, m_output.Call(*m_env, *m_output_get_out));
    t_err = Make<string>(*m_env, m_output.Call(*m_env, *m_output_get_err));
  }, "failed to get captured output");
}

auto Jvm::get_heap_opts() -> pair<string, string> {
  constexpr auto
      XMX_RAM_DEVIDER{4UL},
      MIN_XMX_MB{256UL},
      MAX_XMX_MB{1024UL},

      XMS_RAM_DEVIDER{16UL},
      MIN_XMS_MB{32UL},
      MAX_XMS_MB{MIN_XMX_MB};

  struct sysinfo info{};
  if (sysinfo(&info) != 0) {
    throw runtime_error("failed to get memory info");
  }
  const auto total_ram_mb{info.totalram / (1U << 20U)};

  return {
    "-Xms" + to_string(
        clamp(total_ram_mb / XMS_RAM_DEVIDER, MIN_XMS_MB, MAX_XMS_MB)) + 'M',
    "-Xmx" + to_string(
        clamp(total_ram_mb / XMX_RAM_DEVIDER, MIN_XMX_MB, MAX_XMX_MB)) + 'M'
  };
}
