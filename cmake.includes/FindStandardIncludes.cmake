#
# FindStandardIncludes.cmake
#
# Looks for various "standard" include headers used by many Linux applications.
# These should be verified and the results used when including in our
# proprietary pre-compiled header file.
#
# We assume that "stdio.h" and "stdlib.h" are always available; I have yet to
# come across a C compiler that does not have them.
#
# TODO: provide fallbacks when standard header not found.
#
# ========== ========== ========== ========== ========== ========== ========== ==========

include ( CheckIncludeFile  )
include ( CheckIncludeFiles )

check_include_file ( "assert.h"           HAVE_ASSERT_H             )
check_include_file ( "stdarg.h"           HAVE_STDARG_H             )
check_include_file ( "string.h"           HAVE_STRING_H             )
check_include_file ( "ctype.h"            HAVE_CTYPE_H              )
check_include_file ( "cygwin/if.h"        HAVE_CYGWIN_IF_H          )
check_include_file ( "cygwin/sockios.h"   HAVE_CYGWIN_SOCKIOS_H     )
check_include_file ( "errno.h"            HAVE_ERRNO_H              )
check_include_file ( "fcntl.h"            HAVE_FCNTL_H              )
check_include_file ( "getopt.h"           HAVE_GETOPT_H             )
check_include_file ( "limits.h"           HAVE_LIMITS_H             )
check_include_file ( "netdb.h"            HAVE_NETDB_H              )
check_include_file ( "pthread.h"          HAVE_PTHREAD_H            )
check_include_file ( "signal.h"           HAVE_SIGNAL_H             )
check_include_file ( "time.h"             HAVE_TIME_H               )
check_include_file ( "unistd.h"           HAVE_UNISTD_H             )
check_include_file ( "arpa/inet.h"        HAVE_ARPA_INET_H          )
check_include_file ( "linux/if.h"         HAVE_LINUX_IF_H           )
check_include_file ( "linux/sockios.h"    HAVE_LINUX_SOCKIOS_H      )
check_include_file ( "net/if.h"           HAVE_NET_IF_H             )
check_include_file ( "netinet/in.h"       HAVE_NETINET_IN_H         )

if ( ${CMAKE_SYSTEM} MATCHES "CYGWIN" )
set ( HAVE_NETINET_TCP_H 1 CACHE BOOL "" )
else ()
check_include_file ( "netinet/tcp.h"      HAVE_NETINET_TCP_H        )
endif()

check_include_file ( "sys/ioctl.h"        HAVE_SYS_IOCTL_H          )
check_include_file ( "sys/resource.h"     HAVE_SYS_RESOURCE_H       )
check_include_file ( "sys/select.h"       HAVE_SYS_SELECT_H         )
check_include_file ( "sys/socket.h"       HAVE_SYS_SOCKET_H         )
check_include_file ( "sys/stat.h"         HAVE_SYS_STAT_H           )
check_include_file ( "sys/time.h"         HAVE_SYS_TIME_H           )
check_include_file ( "sys/types.h"        HAVE_SYS_TYPES_H          )
check_include_file ( "sys/wait.h"         HAVE_SYS_WAIT_H           )
