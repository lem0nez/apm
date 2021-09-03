/*
 * Copyright © 2021 Nikita Dudko. All rights reserved.
 * Contacts: <nikita.dudko.95@gmail.com>
 * Licensed under the Apache License, Version 2.0
 */

package com.github.lem0nez.apm;

@SuppressWarnings("serial")
class ExitException extends SecurityException {
    public ExitException(int status) {
        this.status = status;
    }

    public int getStatus() {
        return status;
    }
    private final int status;
}
