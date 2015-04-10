# silverjuke configure.ac script
#
# to create the configure file from configure.ac, run `autoconf`

AC_PREREQ([2.69])
AC_INIT([silverjuke], [15.1], [r10s@b44t.com])

# AC_CONFIG_SRCDIR just checks a single file to make sure, we're in the correct directory
AC_CONFIG_SRCDIR([src/sjbase/mainapp.cpp])

# read configuration from header file instead of the command line;
# this requires a config.h.in which can be created using `autoheader` 
#AC_CONFIG_HEADERS([config.h])

# prepare for using automake, `foreign` is needed as we have not used `automake --add-missing`
# (add [foreign] to AM_INIT_AUTOMAKE to skip ChangeLog, NEWS etc.)
AM_INIT_AUTOMAKE([no-dependencies foreign])
AC_CONFIG_FILES([Makefile])

# Checks for programs
AC_PROG_CXX
AC_PROG_CC
#AC_PROG_MAKE_SET

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lm':
#AC_CHECK_LIB([m], [main])

# Checks for system header files.
#AC_FUNC_ALLOCA
#AC_CHECK_HEADERS([fcntl.h float.h inttypes.h limits.h malloc.h memory.h stddef.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# check for xine 
# (do not define HAVE_XINE_H on success as this would disturb the include wrapper which uses the same define...)
AC_CHECK_HEADER([xine.h], [], [AC_MSG_ERROR([xine.h not found. Try eg. apt-get install libxine-dev])])

# wxWidgets -- we assume that if wx-config is found, wxWidgets is successfully installed.
AC_PATH_PROG(WX_CONFIG, wx-config, no, $PATH:/usr/local/bin )
if [[ "$WX_CONFIG" = "no" ]] ; then
	AC_MSG_ERROR([wx-config not found. Is wxWidgets installed? Is wx-config in your path? Try eg. apt-get install libwxgtk2.8-dev])
fi
WX_CPPFLAGS="`$WX_CONFIG --cppflags`"
WX_CXXFLAGS="`$WX_CONFIG --cxxflags`"
WX_LIBS="`$WX_CONFIG --libs`"

AC_SUBST(WX_CPPFLAGS)
AC_SUBST(WX_CXXFLAGS)
AC_SUBST(WX_LIBS)

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL
#AC_TYPE_INT32_T
#AC_TYPE_INT64_T
#AC_TYPE_MODE_T
#AC_TYPE_OFF_T
#AC_TYPE_SIZE_T
#AC_TYPE_SSIZE_T
#AC_CHECK_MEMBERS([struct stat.st_blksize])
#AC_TYPE_UINT16_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T
#AC_TYPE_UINT8_T

# Checks for library functions.
#AC_FUNC_ERROR_AT_LINE
#AC_FUNC_MALLOC
#AC_FUNC_MKTIME
#AC_FUNC_MMAP
#AC_FUNC_REALLOC
#AC_FUNC_STRTOD
#AC_CHECK_FUNCS([clock_gettime fdatasync floor ftruncate getcwd gettimeofday localeconv localtime_r memmove memset mkdir munmap pow rint sqrt strchr strerror])

AC_OUTPUT


