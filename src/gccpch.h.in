/**
 * @file    gccpch.h
 * @brief   Pre-compiled header file to be included in all other headers.
 * @author  William Clifford
 *
 * Rather than have to include all the various standard library headers in each and every
 * header or code file in the project, we create here a precompiled header that can be
 * included instead. This will give every file access to all of the standard includes in one
 * single include directive.
 *
 * Also want to note that the macro _GNU_SOURCE is defined here before all of the standard
 * library includes. This will provide some additional functions and datatypes to the
 * project, but it may also make it non-portable. Since we are designing only for a
 * GNU/Linux environment, this should not pose any problem.
 *
 * Any type definitions used in the project should also be added here (the socket file
 * descriptor type, for example, or the boolean type). This will help keep the project more
 * organized and the code more readable.
 *
 * */

#ifndef GCCPCH_H__
#define GCCPCH_H__

#include "config.h"

/* Make sure the GNU extensions are available to us. */
#define _GNU_SOURCE

/* Make sure the XOPEN_SOURCE extensions are available to us (for portability). */
#define _XOPEN_SOURCE 600

/* Include here any standard includes that should be made available to any header in the
   application. */

#include <stdio.h>

#include <stdlib.h>

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_LINUX_IF_H
#include <linux/if.h>
#elseif defined(HAVE_CYGWIN_IF_H)
#include <cygwin/if.h>
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#elseif defined(HAVE_CYGWIN_SOCKIOS_H)
#include <cygwin/sockios.h>
#endif

#ifdef HAVE_INET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#ifdef ALSA_FOUND
#include <alsa/asoundlib.h>
#endif

/* ========== ========== ========== ========== ========== ========== ========== ========== */
/*                       Common defines that are not included above.                       */
/* ========== ========== ========== ========== ========== ========== ========== ========== */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Boolean data type     */
/* ---------- ---------- */

#ifndef bool_t
/** Boolean data type, based on an int. */
typedef int bool_t;
#endif

#define ICP_FALSE                       0
#define ICP_TRUE                        (!0)

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* File descriptors for sockets and pipes      */
/* ---------- ---------- ---------- ---------- */

#ifndef fd_t
/** Standard file descriptor type, defined as an integer (int). */
typedef int fd_t;
#endif

#define INVALID_FILE_FD                 ((fd_t) 0xffffffff)
#define INVALID_GENERAL_FD              ((fd_t) 0xffffffff)

#ifndef sock_fd_t
/** Socket file descriptor type, defined as an integer (int). */
typedef int sock_fd_t;
#endif

#define INVALID_SOCKET_FD               ((sock_fd_t) 0xffffffff)

#ifndef pipe_fd_t
/** Pipe file descriptor type, defined as an integer (int). */
typedef int pipe_fd_t;
#endif

#define INVALID_PIPE_FD                 ((pipe_fd_t) 0xffffffff)

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Mutex Handling        */
/* ---------- ---------- */

/* This works in tandem with UNLOCK_MUTEX() defined below. Do not use this macro by itself;
   the code will not compile, and the error messages will look like garbly-gook nonsense
   that has nothing to do with the code itself. This is because pthread_cleanup_push() is
   itself a macro, as is pthread_cleanup_pop(), and they effectively create a braced "{}"
   section of code -- essentially a do {} while (0) loop. Not pairing the macros will break
   the code block that is created and cause all sorts of wierd stuff to happen. */
#ifndef LOCK_MUTEX
#define LOCK_MUTEX(m) \
  pthread_cleanup_push ( (void (*)(void*)) pthread_mutex_unlock, &(m) ); pthread_mutex_lock ( &(m) )
#endif

/* Note that this does not call pthread_mutex_unlock(). By telling pthreads to pop the
   cleanup method, pthread_mutex_unlock() will be called for us since we pushed it onto the
   cleanup stack with LOCK_MUTEX(). Also note that the argument passed to the macro is not
   used. This is ok -- we could have easily not required the argument, but for code
   readability and for consistency the argument is included. Otherwise, we would have
   something like LOCK_MUTEX(m) followed by UNLOCK_MUTEX(), not knowing which mutex is being
   unlocked. This is not normally a problem as more often than not there is only one mutex
   in question at a time. However, this could cause readability problems when multiple
   mutexes (mutices?) are involved. */
#ifndef UNLOCK_MUTEX
#define UNLOCK_MUTEX(m) \
  pthread_cleanup_pop ( 1 )
#endif

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Miscellaneous         */
/* ---------- ---------- */

#ifndef STRINGIFY
#define STRINGIFY(macro_or_str)         STRINGIFY_ARG(macro_or_str)
#define STRINGIFY_ARG(contents)         #contents
#endif

/* Debug Options - Used to check for debug builds, trace enabled, etc. */
#if (defined( _NDEBUG ) || defined( NDEBUG ))
#define ICP_DEBUG_ENABLED               ICP_FALSE
#define ICP_TRACE_ENABLED               ICP_FALSE
#else
#ifdef _DEBUG
#define ICP_DEBUG_ENABLED               ICP_TRUE
#else
#define ICP_DEBUG_ENABLED               ICP_FALSE
#endif
#ifdef _TRACE
#ifndef _DEBUG
#warning _TRACE defined, but _DEBUG not defined; forgot something?
#endif
#define ICP_TRACE_ENABLED               ICP_TRUE
#else
#define ICP_TRACE_ENABLED               ICP_FALSE
#endif
#endif

#ifndef WHEREAMI
#if ICP_TRACE_ENABLED
#define WHEREAMI() \
  fprintf ( stderr, "DEBUG: At %s, %d :::: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__ )
#else
#define WHEREAMI()
#endif
#endif

/* Numeric ranges */
#ifndef BIND_RANGE
#define BIND_RANGE(v, l, h) \
  ( (v) > (h) ? (h) : ( (v) < (l) ? (l) : (v) ) )
#endif

#ifndef IN_RANGE_EXCLUSIVE
#define IN_RANGE_EXCLUSIVE(v, l, h) \
  ( ( (v) > (l) ) && ( (v) < (h) ) )
#endif

#ifndef IN_RANGE_INCLUSIVE
#define IN_RANGE_INCLUSIVE(v, l, h) \
  ( ( (v) >= (l) ) && ( (v) <= (h) ) )
#endif

#ifndef STR_IS_NULL_OR_EMPTY
#define STR_IS_NULL_OR_EMPTY(s) \
  ( !(s) || !(*(s)) )
#endif

/* String Comparison */
#ifndef STR_EQUAL
#define STR_EQUAL(s1, s2) \
  ( strcmp ( (s1), (s2) ) == 0 )
#endif

#ifndef STR_LESS
#define STR_LESS(s1, s2) \
  ( strcmp ( (s1), (s2) ) < 0 )
#endif

/* Assert helpers */
#ifdef HAVE_ASSERT_H
#if (defined( _NDEBUG ) || defined( NDEBUG ))
#define ASSERT_EXIT_FALSE( test )           do { if ( !( test ) ) return ICP_FALSE; } while ( 0 )
#define ASSERT_EXIT_NULL( test, dtype )     do { if ( !( test ) ) return ( dtype ) 0; } while ( 0 )
#define ASSERT_EXIT_VOID( test )            do { if ( !( test ) ) return; } while ( 0 )
#else
#define ASSERT_EXIT_FALSE( test )           assert ( test )
#define ASSERT_EXIT_NULL( test, dtype )     assert ( test )
#define ASSERT_EXIT_VOID( test )            assert ( test )
#endif
#endif

#endif /* GCCPCH_H__ */
