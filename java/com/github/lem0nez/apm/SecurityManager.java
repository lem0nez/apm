/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

package com.github.lem0nez.apm;
import java.security.Permission;

// It must be set AFTER calling Output.redirect and BEFORE calling Tool.run.
public class SecurityManager extends java.lang.SecurityManager {
    @Override
    public void checkExit(int status) {
        // Throw the exception instead of terminating a process when Runtime.exit is called.
        throw new ExitException(status);
    }

    @Override
    public void checkPermission(Permission perm) {
        if (perm.getName().equals("setSecurityManager")) {
            // Don't allow to override this SecurityManager.
            throw new SecurityException("setSecurityManager isn't allowed");
        }
        if (perm.getName().equals("setIO")) {
            // Class Output must have control over the standard output streams.
            throw new SecurityException("setIO isn't allowed");
        }
    }
}
