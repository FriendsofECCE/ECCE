#!/bin/sh
# CPack's strip for the Linux packages: drop the debug sections but keep the
# symbol table, so a crash backtrace (coredumpctl, gdb) names the functions.
# About +25 % on the binaries, against ~15x for keeping -g.
exec strip --strip-debug "$@"
