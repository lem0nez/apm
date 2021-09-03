/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

package com.github.lem0nez.apm;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;

public class Output {
    // This function must be called before calling getters.
    public static void redirect() {
        // Pass true to enable automatic buffer flushing.
        System.setOut(new PrintStream(out, true));
        System.setErr(new PrintStream(err, true));
    }

    public static void reset() {
        out.reset();
        err.reset();
    }

    public static String getOut() {
        return out.toString();
    }
    public static String getErr() {
        return err.toString();
    }

    private static final ByteArrayOutputStream
            out = new ByteArrayOutputStream(),
            err = new ByteArrayOutputStream();
}
