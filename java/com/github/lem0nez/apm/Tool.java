/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

package com.github.lem0nez.apm;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.jar.Attributes;
import java.util.jar.JarFile;

public class Tool {
    public Tool(String jarPath) throws Exception {
        // Pass false to disable verification of signed JAR files.
        final JarFile jar = new JarFile(jarPath, false);
        final Attributes mainAttrs = jar.getManifest().getMainAttributes();
        final Class<?> mainClass = Class.forName(mainAttrs.getValue("Main-Class"));
        mainMethod = mainClass.getMethod("main", String[].class);
    }

    public int run(String[] args) throws Exception {
        try {
            // Pass null as it's a static method.
            mainMethod.invoke(null, (Object) args);
        } catch (InvocationTargetException e) {
            final Throwable cause = e.getCause();
            if (cause != null) {
                if (cause instanceof ExitException) {
                    return ((ExitException) cause).getStatus();
                }
                throw new Exception(cause);
            }
            throw e;
        }
        return 0;
    }

    private final Method mainMethod;
}
