/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

template<typename R> auto Jvm::safe_java_exec(const std::function<R()>& t_fun,
    const std::string_view t_err_msg) const -> R {
  using namespace std;
  using namespace jni;

  try {
    return t_fun();
  } catch (const PendingJavaException&) {
    const Local<Object<Throwable>>
        throwable_obj(*m_env, ExceptionOccurred(*m_env));
    // An exception must be cleared before calling JNI functions again.
    ExceptionClear(*m_env);

    string msg(t_err_msg);
    try {
      const auto throwable{Class<Throwable>::Find(*m_env)};
      const auto to_string{throwable.GetMethod<String()>(*m_env, "toString")};
      msg += " (" + Make<string>(*m_env,
             throwable_obj.Call(*m_env, to_string)) + ')';
    } catch (const PendingJavaException&) {
      ExceptionClear(*m_env);
    }
    throw java_error(msg);
  }
}
