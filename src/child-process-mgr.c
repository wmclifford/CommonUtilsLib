/**
 * @file    child-process-mgr.c
 * @author  William Clifford
 *
 **/

#include "child-process-mgr.h"

#define CATEGORY_NAME "childprocmgr"
#include "logging-svc.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Constants  */
/* ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Type definitions and structures  */
/* ---------- ---------- ---------- */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// monitored_proc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _monitored_proc {
  struct _monitored_proc *  next;
  struct _monitored_proc *  prev;
  struct _child_proc *      child;
  bool_t                    owned_by_mgr;
} monitored_proc_t, * p_monitored_proc_t;

#define SIZE_monitored_proc             (sizeof( struct _monitored_proc ))
#define NEW_monitored_proc()            ( (p_monitored_proc_t) malloc ( sizeof( struct _monitored_proc ) ) )
#define NIL_monitored_proc              ( (p_monitored_proc_t) 0 )
#define AS_PTR_monitored_proc(vp)       ( (p_monitored_proc_t) vp )

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// child_proc_mgr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _child_proc_mgr {
  p_io_scheduler_task_t     monitor_task;
  p_monitored_proc_t        monitored_procs_head;
  pthread_mutex_t           monitored_procs_mutex;
} child_proc_mgr_t /*, * p_child_proc_mgr_t*/ ;

#define SIZE_child_proc_mgr             (sizeof( struct _child_proc_mgr ))
#define NEW_child_proc_mgr()            ( (p_child_proc_mgr_t) malloc ( sizeof( struct _child_proc_mgr ) ) )
#define NIL_child_proc_mgr              ( (p_child_proc_mgr_t) 0 )
#define AS_PTR_child_proc_mgr(vp)       ( (p_child_proc_mgr_t) vp )

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Shared (global) variables        */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local function prototypes        */
/* ---------- ---------- ---------- */

static p_monitored_proc_t find_monitored_proc ( p_child_proc_mgr_t cpmgr, pid_t pid );

static void monitored_proc_destroy ( p_monitored_proc_t mp );

static p_monitored_proc_t monitored_proc_init ( p_child_proc_t child, bool_t owns_child );

static bool_t on_monitor_timer ( p_io_scheduler_task_t task, int errcode );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Module variables      */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

void
child_proc_mgr_destroy ( struct _child_proc_mgr * cpmgr )
{
  p_monitored_proc_t mp;
  
  ASSERT_EXIT_VOID( cpmgr );  
  child_proc_mgr_stop ( cpmgr );
  mp = cpmgr->monitored_procs_head->next;
  while ( mp && ( mp != cpmgr->monitored_procs_head ) ) {
    cpmgr->monitored_procs_head->next = mp->next;
    monitored_proc_destroy ( mp );
    mp = cpmgr->monitored_procs_head->next;
  }
  free ( cpmgr->monitored_procs_head );
  pthread_mutex_destroy ( &( cpmgr->monitored_procs_mutex ) );
  free ( cpmgr );
}

struct _child_proc_mgr *
child_proc_mgr_init ( void )
{
  p_child_proc_mgr_t rv = NEW_child_proc_mgr();
  p_monitored_proc_t mp;
  if ( rv ) {
    rv->monitor_task = NIL_IO_SCHEDULER_TASK;
    mp = NEW_monitored_proc();
    if ( !( mp ) ) {
      free ( rv );
      return NIL_child_proc_mgr;
    }
    mp->prev = mp->next = mp;
    mp->child = NIL_child_proc;
    mp->owned_by_mgr = CMNUTIL_FALSE;
    rv->monitored_procs_head = mp;
    pthread_mutex_init( &( rv->monitored_procs_mutex ), (const pthread_mutexattr_t*) 0 );
  }
  return rv;
}

bool_t
child_proc_mgr_monitor_child ( struct _child_proc_mgr * cpmgr, p_child_proc_t child )
{
  bool_t rv = CMNUTIL_TRUE;
  p_monitored_proc_t mp, last;
  
  ASSERT_EXIT_FALSE( cpmgr );
  ASSERT_EXIT_FALSE( child );
  
  LOCK_MUTEX( cpmgr->monitored_procs_mutex );
  
  // See if we already have a monitor for this child process.
  mp = cpmgr->monitored_procs_head->next;
  while ( mp && ( mp != cpmgr->monitored_procs_head ) ) {
    if ( mp->child->pid == child->pid )
      break;
    mp = mp->next;
  }
  if ( mp != cpmgr->monitored_procs_head ) {
    // Found a monitor for this PID ... is it the same pointer? If so, we can jump out and return TRUE.
    if ( mp->child != child ) {
      LOGSVC_WARNING( "This child process is already being monitored; refusing to add another monitor." );
      rv = CMNUTIL_FALSE;
    }
  }
  else {
    // Not found ... we can add it.
    mp = monitored_proc_init ( child, CMNUTIL_FALSE );
    if ( mp ) {
      last = cpmgr->monitored_procs_head->prev;
      last->next = mp;
      mp->prev = last;
      mp->next = cpmgr->monitored_procs_head;
      cpmgr->monitored_procs_head->prev = mp;
      LOGSVC_INFO( "Monitoring child process (%d).", child->pid );
    }
    else {
      LOGSVC_ERROR( "Unable to create monitor for this child process (%d).", child->pid );
      rv = CMNUTIL_FALSE;
    }
  }
  
  UNLOCK_MUTEX( cpmgr->monitored_procs_mutex );
  
  return rv;
}

bool_t
child_proc_mgr_monitor_pid ( struct _child_proc_mgr * cpmgr,
                             pid_t pid, fd_t fd, void * userdata, child_proc_exited_t on_pid_exit )
{
  bool_t rv = CMNUTIL_TRUE;
  p_monitored_proc_t mp, last;
  p_child_proc_t child;
  
  ASSERT_EXIT_FALSE( cpmgr );
  ASSERT_EXIT_FALSE( (pid > 1) );
  
  LOCK_MUTEX( cpmgr->monitored_procs_mutex );
  
  // See if we already have a monitor for this child process.
  mp = cpmgr->monitored_procs_head->next;
  while ( mp && ( mp != cpmgr->monitored_procs_head ) ) {
    if ( mp->child->pid == pid )
      break;
    mp = mp->next;
  }
  if ( mp != cpmgr->monitored_procs_head ) {
    // Found a monitor for this PID ...
    LOGSVC_WARNING( "This child process is already being monitored; refusing to add another monitor." );
    rv = CMNUTIL_FALSE;
  }
  else {
    // Not found ... we can add it.
    child = child_proc_init_full ( pid, fd, userdata, on_pid_exit );
    mp = monitored_proc_init ( child, CMNUTIL_TRUE );
    if ( mp ) {
      last = cpmgr->monitored_procs_head->prev;
      last->next = mp;
      mp->prev = last;
      mp->next = cpmgr->monitored_procs_head;
      cpmgr->monitored_procs_head->prev = mp;
      LOGSVC_INFO( "Monitoring child process (%d).", child->pid );
    }
    else {
      LOGSVC_ERROR( "Unable to create monitor for this child process (%d).", pid );
      if ( child )
        child_proc_destroy ( child );
      rv = CMNUTIL_FALSE;
    }
  }
  
  UNLOCK_MUTEX( cpmgr->monitored_procs_mutex );
  
  return rv;
}

bool_t
child_proc_mgr_start ( struct _child_proc_mgr * cpmgr, p_io_scheduler_t scheduler )
{
  ASSERT_EXIT_FALSE( cpmgr );
  ASSERT_EXIT_FALSE( scheduler );
  
  if ( cpmgr->monitor_task != NIL_IO_SCHEDULER_TASK )
    child_proc_mgr_stop ( cpmgr );
  
  cpmgr->monitor_task =
    io_sched_create_timer_task ( scheduler, IO_SCHEDULER_TIME_ONE_SECOND, (void*) cpmgr, on_monitor_timer );
  
  if ( !( io_sched_schedule_task ( cpmgr->monitor_task ) ) ) {
    LOGSVC_ERROR( "Failed to create/schedule monitor I/O task." );
    return CMNUTIL_FALSE;
  }
  
  return CMNUTIL_TRUE;
}

void
child_proc_mgr_stop ( struct _child_proc_mgr * cpmgr )
{
  ASSERT_EXIT_VOID( cpmgr );
  
  if ( cpmgr->monitor_task != NIL_IO_SCHEDULER_TASK ) {
    LOGSVC_DEBUG( "Stopping monitor I/O task." );
    io_sched_unschedule_task ( cpmgr->monitor_task );
    cpmgr->monitor_task = NIL_IO_SCHEDULER_TASK;
  }
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

static p_monitored_proc_t
find_monitored_proc ( p_child_proc_mgr_t cpmgr, pid_t pid )
{
  p_monitored_proc_t rv;
  
  ASSERT_EXIT_NULL( cpmgr, p_monitored_proc_t );
  ASSERT_EXIT_NULL( (pid > 1), p_monitored_proc_t );
  ASSERT_EXIT_NULL( cpmgr->monitored_procs_head, p_monitored_proc_t );
  
  LOCK_MUTEX( cpmgr->monitored_procs_mutex );
  rv = cpmgr->monitored_procs_head->next;
  while ( rv && ( rv != cpmgr->monitored_procs_head ) ) {
    if ( rv->child->pid == pid )
      break;
    rv = rv->next;
  }
  UNLOCK_MUTEX( cpmgr->monitored_procs_mutex );
  
  // If we made our way back to the head item, then the PID was not found in the list.
  if ( rv == cpmgr->monitored_procs_head )
    rv = NIL_monitored_proc;
  
  return rv;
}

static void
monitored_proc_destroy ( p_monitored_proc_t mp )
{
  ASSERT_EXIT_VOID( mp );
  if ( mp->owned_by_mgr )
    child_proc_destroy ( mp->child );
  free ( mp );
}

static p_monitored_proc_t
monitored_proc_init ( p_child_proc_t child, bool_t owns_child )
{
  ASSERT_EXIT_NULL( child, p_monitored_proc_t );
  p_monitored_proc_t rv = NEW_monitored_proc();
  if ( rv ) {
    rv->next = rv->prev = NIL_monitored_proc;
    rv->child = child;
    rv->owned_by_mgr = owns_child;
  }
  return rv;
}

static bool_t
on_monitor_timer ( p_io_scheduler_task_t task, int errcode )
{
  //
  // Basically we want to see if any child process has exited, and if so, is it one
  // that we are monitoring. If we are monitoring it, we need to remove it from our
  // monitor list and call its on_exit callback (if it has one). This repeats for
  // each child process that has exited.
  //
  pid_t child_pid;
  int status;
  p_child_proc_mgr_t cpmgr = AS_PTR_child_proc_mgr( task->user_data );
  p_monitored_proc_t mp;
  
  do {
    // See if any child process has exited (get its process id).
    child_pid = waitpid ( 0, &status, WNOHANG );
    
    if ( child_pid == -1 )
      LOGSVC_ERROR( "on_monitor_timer(): waitpid() returned an error: '%s'", strerror ( errno ) );
    else if ( child_pid > 0 ) {
      mp = find_monitored_proc ( cpmgr, child_pid );
      if ( mp ) {
        // Unlink it from the list.
        LOCK_MUTEX( cpmgr->monitored_procs_mutex );
        mp->prev->next = mp->next;
        mp->next->prev = mp->prev;
        UNLOCK_MUTEX( cpmgr->monitored_procs_mutex );
        // Call its on_exit, then get rid of the monitor instance.
        if ( mp->child->on_exit )
          mp->child->on_exit ( mp->child, status );
        monitored_proc_destroy ( mp );
      }
    }
    
  } while ( child_pid > 0 );
  
  return IO_SCHEDULER_TASK_INCOMPLETE;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
