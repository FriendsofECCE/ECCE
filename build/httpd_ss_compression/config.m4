dnl modules enabled in this directory by default
dnl APACHE_MODULE(name, helptext[, objects[, structname[, default[, config]]]])

APACHE_MODPATH_INIT(ss_compression)

APR_ADDTO(LT_LDFLAGS,-export-dynamic)

APACHE_MODULE(ss_compression, Server side compression support, , , no, [
  AC_ARG_WITH(z, APACHE_HELP_STRING(--with-z=DIR,use a specific zlib library),
  [ ap_zlib_base="$withval" ])
  if test "$enable_ss_compression" != "no"; then
    ap_save_includes=$INCLUDE
    ap_save_ldflags=$LDFLAGS
    ap_save_libs=$LIBS
    ap_save_cppflags=$CPPFLAGS
    APR_ADDTO(INCLUDES, [-I${ap_zlib_base}/include])
    dnl put in CPPFLAGS temporarily so that AC_TRY_LINK below will work
    CPPFLAGS="$CPPFLAGS $INCLUDES"
    APR_ADDTO(LDFLAGS, [-L${ap_zlib_base}/lib])
    if test "x$ap_platform_runtime_link_flag" != "x"; then
       APR_ADDTO(LDFLAGS, [$ap_platform_runtime_link_flag${ap_zlib_base}/lib])
    fi
    APR_ADDTO(LIBS, [-lz])
    AC_MSG_CHECKING([for zlib library])
    AC_TRY_LINK([#include <zlib.h>], [int i = Z_OK;], 
    [AC_MSG_RESULT(found) 
     AC_CHECK_HEADERS(zutil.h)],
    [AC_MSG_RESULT(not found)
     enable_ss_compression=no
     INCLUDES=$ap_save_includes
     LDFLAGS=$ap_save_ldflags
     LIBS=$ap_save_libs])
    CPPFLAGS=$ap_save_cppflags
  fi
])

APACHE_MODPATH_FINISH
