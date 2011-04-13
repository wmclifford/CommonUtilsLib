/**
 * @file logging-svc.c
 * @author William Clifford
 *
 **/

#include "logging-svc.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Shared (global) variables        */
/* ---------- ---------- ---------- */

bool_t g_logsvc_logging_on = ICP_TRUE;
bool_t g_logsvc_debug_on = ICP_FALSE;
bool_t g_logsvc_trace_on = ICP_FALSE;

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local function prototypes        */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Module variables      */
/* ---------- ---------- */

#ifndef _USE_LOG4C
static const char * priorities[] = {
  "FATAL",
    "ALERT",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG",
    "TRACE"
};
#define PRIORITYNAME( lvl )             (priorities[(lvl) >> 8])
int max_priority_level = LOGSVC_PRIORITY_INFO;
#endif

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

void
logsvc_log ( const char * category_name, int priority_level, const char * msgfmt, ... )
{
  va_list va;
  
  if ( !( g_logsvc_logging_on ) )
    return;
  
#ifdef _USE_LOG4C
  const log4c_category_t * category = log4c_category_get ( category_name );
  if ( (category) && log4c_category_is_priority_enabled ( category, priority_level ) ) {
    va_start ( va, msgfmt );
    log4c_category_vlog ( category, priority_level, msgfmt, va );
    va_end ( va );
  }
#else
  char buffer[1024];
  struct timeval curtimeval;
  struct tm gmt_cur_time;
  
  if ( priority_level > max_priority_level )
    return;
  
  memset ( buffer, 0, 1024 );
  va_start ( va, msgfmt );
  vsnprintf ( buffer, 1023, msgfmt, va );
  va_end ( va );
  gettimeofday ( &curtimeval, NULL );
  gmtime_r ( &(curtimeval.tv_sec), &gmt_cur_time );
  fprintf (
            stderr,
            "[%6d] %04d%02d%02d %02d:%02d:%02d.%03ld %-8s %s- %s\n",
            getpid (),
            gmt_cur_time.tm_year + 1900, gmt_cur_time.tm_mon + 1, gmt_cur_time.tm_mday,
            gmt_cur_time.tm_hour, gmt_cur_time.tm_min, gmt_cur_time.tm_sec,
            curtimeval.tv_usec / 1000,
            PRIORITYNAME( priority_level ),
            category_name,
            buffer
          );
#endif
}

void
logsvc_start ( void )
{
  /* Just in case it was disabled at the command line. */
  if ( !( g_logsvc_logging_on ) )
    return;
  
#ifdef _USE_LOG4C
  
  if ( log4c_init () ) {
    /* something's wrong ... */
    fprintf ( stderr, "LOG4C failed to initialize - logging disabled.\n" );
    g_logsvc_logging_on = ICP_FALSE;
    return;
  }
  else {
    /* Configure log4c - use some default settings. */
    log4c_category_t * root_category = log4c_category_get ( "root" );
    log4c_appender_t * stderr_appender = log4c_appender_get ( "stderr" );
    log4c_category_set_appender ( root_category, stderr_appender );
    log4c_appender_set_layout ( stderr_appender, log4c_layout_get ( "dated" ) );
    if ( g_logsvc_trace_on )
      log4c_category_set_priority ( root_category, LOGSVC_PRIORITY_TRACE );
    else if ( g_logsvc_debug_on )
      log4c_category_set_priority ( root_category, LOGSVC_PRIORITY_DEBUG );
    else
      log4c_category_set_priority ( root_category, LOGSVC_PRIORITY_INFO );
  }
  
#else
  
  if ( g_logsvc_trace_on ) {
#if ICP_TRACE_ENABLED
    max_priority_level = LOGSVC_PRIORITY_TRACE;
#else
    fprintf ( stderr, "Warning: requested TRACE logging level in non-trace-enabled build.\nIgnoring.\n" );
#endif
  }
  else if ( g_logsvc_debug_on ) {
#if ICP_DEBUG_ENABLED
    max_priority_level = LOGSVC_PRIORITY_DEBUG;
#else
    fprintf ( stderr, "Warning: requested DEBUG logging level in non-debug-enabled build.\nIgnoring.\n" );
#endif
  }
#endif
}

void
logsvc_stop ( void )
{
#ifdef _USE_LOG4C
  log4c_fini ();
#else
  /* Nothing to do here */
#endif
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
