/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <jni/jni.hpp>
#include "sdk.hpp"

namespace jvm_tools {
  using flags_t = std::byte;
  static constexpr flags_t
      JAVAC{1U},
      D8{1U << 1U},
      APKSIGNER{1U << 2U},
      ALL{JAVAC | D8 | APKSIGNER};
}

class java_error : public std::runtime_error {
public:
  explicit java_error(const std::string_view what_arg):
      runtime_error(std::string(what_arg)) {}
};

/*
 * IMPORTANT NOTES:
 * 1. due to JNI limitations, only one JVM can be created per a process;
 * 2. tools must only be called from a thread that instantiated Jvm.
 */
class Jvm {
  using tool_t = auto (const std::vector<std::string>& args,
                 std::string& out, std::string& err) const -> int;
public:
  // Throws an exception on failure.
  Jvm(jvm_tools::flags_t init_tools, std::shared_ptr<const Sdk> sdk);
  ~Jvm();

  Jvm(const Jvm&) = delete;
  auto operator=(const Jvm&) -> Jvm& = delete;
  Jvm(Jvm&&) = default;
  auto operator=(Jvm&&) -> Jvm& = default;

  // If a tool isn't initialized by the
  // constructor, runtime_error will be thrown.
  tool_t
      javac,
      d8,
      apksigner;

private:
  struct Throwable {
    static constexpr auto Name() { return "java/lang/Throwable"; }
  };

  struct InputStream {
    static constexpr auto Name() { return "java/io/InputStream"; }
  };
  struct OutputStream {
    static constexpr auto Name() { return "java/io/OutputStream"; }
  };
  struct JavaCompiler {
    static constexpr auto Name() { return "javax/tools/JavaCompiler"; }
  };

  struct Output {
    static constexpr auto Name() { return "com/github/lem0nez/apm/Output"; }
  };
  struct Tool {
    static constexpr auto Name() { return "com/github/lem0nez/apm/Tool"; }
  };

  static constexpr auto JNI_VERSION{JNI_VERSION_1_8};

  // Redirects Java's standard output streams.
  void redirect_output();
  void set_security_manager() const;
  void init_javac();
  void init_android_tools(
      jvm_tools::flags_t tools, std::shared_ptr<const Sdk> sdk);

  [[nodiscard]] auto make_args(const std::vector<std::string>& args) const ->
      jni::Local<jni::Array<jni::String>>;
  void reset_output() const;
  // Assigns captured outputs since the last reset_output call.
  void get_output(std::string& out, std::string& err) const;

  // If PendingJavaException will be caught, java_error with the
  // provided error message and exception details will be thrown.
  template<typename R> auto safe_java_exec(
      const std::function<R()>& fun, std::string_view err_msg) const -> R;
  // Returns options pair of minimum and maximum heap sizes.
  [[nodiscard]] static auto get_heap_opts() ->
      std::pair<std::string, std::string>;

  // Memory management isn't required, since
  // content of this pointers are owned by JNI.
  jni::JavaVM* m_vm{};
  jni::JNIEnv* m_env{};

  jni::Local<jni::Class<Output>> m_output;
  std::unique_ptr<const jni::StaticMethod<Output, void()>> m_output_reset;
  std::unique_ptr<const jni::StaticMethod<Output, jni::String()>>
      m_output_get_out,
      m_output_get_err;

  jni::Local<jni::Object<JavaCompiler>> m_javac_obj;
  using javac_run_t = jni::jint(
      // Parameters: in, out, err, arguments.
      jni::Object<InputStream>, jni::Object<OutputStream>,
      jni::Object<OutputStream>, jni::Array<jni::String>);
  std::unique_ptr<const jni::Method<JavaCompiler, javac_run_t>> m_javac_run;

  jni::Local<jni::Object<Tool>>
      m_d8_obj,
      m_apksigner_obj;
  std::unique_ptr<const jni::Method<Tool,
                  jni::jint(jni::Array<jni::String>)>> m_tool_run;
};

#include "jvm.inl"
