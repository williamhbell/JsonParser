dnl library version number
m4_define([rel_major],1)
m4_define([rel_minor],0)
m4_define([rel_micro],0)
m4_define([rel_version],[rel_major.rel_minor.rel_micro])

dnl package name
m4_define([package_name],jsonparser)


AC_INIT([package_name],[rel_version])
AC_PREREQ([2.68])
AC_CONFIG_SRCDIR([src/json_common.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 -Wall -Werror])
AC_CONFIG_FILES([Makefile])
AC_PROG_CC
AC_OUTPUT

dnl Enable silent rules, when available (automake 1.11 or later).
dnl m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl AC_CONFIG_HEADER([config.h])

dnl CC_CHECK_CFLAGS_APPEND([-Wall -Wextra])

dnl AC_CONFIG_FILES([Makefile src/Makefile])
dnlAC_OUTPUT([Makefile])
