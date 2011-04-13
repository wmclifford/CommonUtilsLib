/**
 * @file    logging-svc.h
 * @author  William Clifford
 * 
 * Simple, yet very useful logging service. Has the ability to optionally use the log4c
 * library or simply log messages to stderr in a predefined format. Users can set the
 * verbosity level by enabling the debug and trace priority levels, and can even disable
 * logging completely by setting the g_logsvc_logging_on flag to false.
 * 
 * @todo    One thing that would be nice to have is the ability to turn on other log recipients
 *          (syslog, network socket, etc.), most likely when log4c is in use. Not sure if it
 *          would be worthwhile to try and implement such things in the "standard" logging.
 **/

#ifndef LOGGING_SVC_H__
#define LOGGING_SVC_H__

/* Include the precompiled header for all the standard library includes and project-wide
   definitions. */
#include "gccpch.h"

/* Allow the user of this service to set the category name before including this header,
   rather than having to pass it each time one of the LOGSVC_* macros is called. */
#ifndef CATEGORY_NAME
#define CATEGORY_NAME "root"
#endif

/* Make sure the library is available and that we want to make use of it. */
#if defined(LOG4C_FOUND) && defined(USE_LOG4C)
#define _USE_LOG4C
#endif

#ifdef _USE_LOG4C
#include <log4c.h>
#endif

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#ifdef _USE_LOG4C
#define LOGSVC_PRIORITY_FATAL           LOG4C_PRIORITY_FATAL
#define LOGSVC_PRIORITY_ALERT           LOG4C_PRIORITY_ALERT
#define LOGSVC_PRIORITY_CRITICAL        LOG4C_PRIORITY_CRIT
#define LOGSVC_PRIORITY_ERROR           LOG4C_PRIORITY_ERROR
#define LOGSVC_PRIORITY_WARNING         LOG4C_PRIORITY_WARN
#define LOGSVC_PRIORITY_NOTICE          LOG4C_PRIORITY_NOTICE
#define LOGSVC_PRIORITY_INFO            LOG4C_PRIORITY_INFO
#define LOGSVC_PRIORITY_DEBUG           LOG4C_PRIORITY_DEBUG
#define LOGSVC_PRIORITY_TRACE           LOG4C_PRIORITY_TRACE
#else
#define LOGSVC_PRIORITY_FATAL           0x0000
#define LOGSVC_PRIORITY_ALERT           0x0100
#define LOGSVC_PRIORITY_CRITICAL        0x0200
#define LOGSVC_PRIORITY_ERROR           0x0300
#define LOGSVC_PRIORITY_WARNING         0x0400
#define LOGSVC_PRIORITY_NOTICE          0x0500
#define LOGSVC_PRIORITY_INFO            0x0600
#define LOGSVC_PRIORITY_DEBUG           0x0700
#define LOGSVC_PRIORITY_TRACE           0x0800
#endif

/* ---------- ---------- ---------- ---------- */
/* Common logging levels -- helper macros      */
/* ---------- ---------- ---------- ---------- */

/* Caller uses the default category name, set before including this header. */

#define LOGSVC_FATAL( msgfmt, ... ) \
logsvc_log ( CATEGORY_NAME, LOGSVC_PRIORITY_FATAL, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_CRITICAL( msgfmt, ... ) \
logsvc_log ( CATEGORY_NAME, LOGSVC_PRIORITY_CRITICAL, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_ERROR( msgfmt, ... ) \
logsvc_log ( CATEGORY_NAME, LOGSVC_PRIORITY_ERROR, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_WARNING( msgfmt, ... ) \
logsvc_log ( CATEGORY_NAME, LOGSVC_PRIORITY_WARNING, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_NOTICE( msgfmt, ... ) \
logsvc_log ( CATEGORY_NAME, LOGSVC_PRIORITY_NOTICE, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_INFO( msgfmt, ... ) \
logsvc_log ( CATEGORY_NAME, LOGSVC_PRIORITY_INFO, msgfmt, ##__VA_ARGS__ )

#if CMNUTIL_DEBUG_ENABLED
#define LOGSVC_DEBUG( msgfmt, ... ) \
logsvc_log ( CATEGORY_NAME, LOGSVC_PRIORITY_DEBUG, msgfmt, ##__VA_ARGS__ )
#else
#define LOGSVC_DEBUG( msgfmt, ... )
#endif

#if CMNUTIL_TRACE_ENABLED
#define LOGSVC_TRACE( msgfmt, ... ) \
logsvc_log ( CATEGORY_NAME, LOGSVC_PRIORITY_TRACE, msgfmt, ##__VA_ARGS__ )
#else
#define LOGSVC_TRACE( msgfmt, ... )
#endif

/* Caller specifies the category name at call time. */

#define LOGSVC_CATEGORY_FATAL( category_name, msgfmt, ... ) \
  logsvc_log ( category_name, LOGSVC_PRIORITY_FATAL, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_CATEGORY_CRITICAL( category_name, msgfmt, ... ) \
  logsvc_log ( category_name, LOGSVC_PRIORITY_CRITICAL, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_CATEGORY_ERROR( category_name, msgfmt, ... ) \
  logsvc_log ( category_name, LOGSVC_PRIORITY_ERROR, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_CATEGORY_WARNING( category_name, msgfmt, ... ) \
  logsvc_log ( category_name, LOGSVC_PRIORITY_WARNING, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_CATEGORY_NOTICE( category_name, msgfmt, ... ) \
  logsvc_log ( category_name, LOGSVC_PRIORITY_NOTICE, msgfmt, ##__VA_ARGS__ )

#define LOGSVC_CATEGORY_INFO( category_name, msgfmt, ... ) \
  logsvc_log ( category_name, LOGSVC_PRIORITY_INFO, msgfmt, ##__VA_ARGS__ )

#if CMNUTIL_DEBUG_ENABLED
#define LOGSVC_CATEGORY_DEBUG( category_name, msgfmt, ... ) \
  logsvc_log ( category_name, LOGSVC_PRIORITY_DEBUG, msgfmt, ##__VA_ARGS__ )
#else
#define LOGSVC_CATEGORY_DEBUG( category_name, msgfmt, ... )
#endif

#if CMNUTIL_TRACE_ENABLED
#define LOGSVC_CATEGORY_TRACE( category_name, msgfmt, ... ) \
  logsvc_log ( category_name, LOGSVC_PRIORITY_TRACE, msgfmt, ##__VA_ARGS__ )
#else
#define LOGSVC_CATEGORY_TRACE( category_name, msgfmt, ... )
#endif

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

extern bool_t g_logsvc_logging_on;
extern bool_t g_logsvc_debug_on;
extern bool_t g_logsvc_trace_on;

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

void logsvc_log ( const char * category_name, int priority_level, const char * msgfmt, ... );

void logsvc_start ( void );

void logsvc_stop ( void );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* LOGGING_SVC_H__ */
