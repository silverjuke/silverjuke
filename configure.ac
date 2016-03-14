# Process this file with autoconf to produce a configure script.
#
# Copyright © 2015 Björn Petersen
# Copyright © 2015 Dr. Tobias Quathamer
#
# This file is part of silverjuke.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Set up autoconf
AC_PREREQ([2.69])
AC_INIT([silverjuke], [16.3.3], [r10s@b44t.com])
AC_CONFIG_MACRO_DIR([m4])

# AC_CONFIG_SRCDIR just checks a single file to make sure, we're in the correct directory
AC_CONFIG_SRCDIR([src/sjbase/mainapp.cpp])

# Set up automake
AM_INIT_AUTOMAKE([1.13 foreign subdir-objects silent-rules no-dist-gzip dist-xz])
AM_MAINTAINER_MODE

# Set up gettext
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18])
#AC_DEFINE([GETTEXT_PACKAGE], ["silverjuke"], [Gettext package name.])

# Checks for programs
AC_PROG_CXX
AC_PROG_CC
PKG_PROG_PKG_CONFIG

# Detect rst2man, needed for building the manpage
AC_PATH_PROG([RST2MAN], [rst2man], [no])
if test x"$RST2MAN" == x"no" ; then
    AC_MSG_ERROR([rst2man is required.])
fi

# Detect the needed libraries
PKG_CHECK_MODULES([ZLIB], [zlib])
PKG_CHECK_MODULES([SQLITE3], [sqlite3])
PKG_CHECK_MODULES([GST], [gstreamer-1.0 gstreamer-plugins-base-1.0])
PKG_CHECK_MODULES([GL], [gl])

# Check for endian.h to determine endianness for "src/see" library
AC_CHECK_HEADERS([endian.h])

# wxWidgets -- we assume that if wx-config is found, wxWidgets is successfully installed.
AC_PATH_PROG([WX_CONFIG], [wx-config], [no], [$PATH:/usr/local/bin])
if test x"$WX_CONFIG" = x"no" ; then
	AC_MSG_ERROR([wx-config not found. Is wxWidgets installed? Is wx-config in your path? Try eg. apt-get install libwxgtk3.0-dev])
fi
WX_CPPFLAGS="`$WX_CONFIG --cppflags`"
WX_CXXFLAGS="`$WX_CONFIG --cxxflags`"
WX_LIBS="`$WX_CONFIG --libs std,gl`"

AC_SUBST(WX_CPPFLAGS)
AC_SUBST(WX_CXXFLAGS)
AC_SUBST(WX_LIBS)

# Write configuration files
AC_CONFIG_FILES([
	Makefile
	po/Makefile.in
])

# Terminate script
AC_OUTPUT
