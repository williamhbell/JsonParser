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
AM_INIT_AUTOMAKE([foreign subdir-objects 1.11 -Wall -Werror])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_FILES([Makefile])
AC_PROG_CC
AM_PROG_AR
AC_OUTPUT
LT_INIT()
