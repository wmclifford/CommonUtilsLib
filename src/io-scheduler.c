/**
 * @file io-scheduler.c
 * @author William Clifford
 *
 **/

#include "io-scheduler.h"

#define CATEGORY_NAME "io-scheduler"
#include "logging-svc.h"

/* Undefining this here, simply because it starts to cause performance issues when trying to debug. */
#undef LOGSVC_TRACE
#define LOGSVC_TRACE( fmt, ... )

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Shared (global) variables        */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local function prototypes        */
/* ---------- ---------- ---------- */

static void io_sched_destroy_task ( p_io_scheduler_task_t io_task );

static void * io_sched_threadfn ( void * ud );

static inline void inl_io_sched_populate_expire_time ( p_io_scheduler_task_t io_task );

static inline void inl_io_sched_pump ( p_io_scheduler_t scheduler );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Module variables      */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

/**
 * Creates a read-only task for the given file descriptor; will wait only time_out microseconds
 * before calling the read callback method with a timeout error.
 **/
p_io_scheduler_task_t
io_sched_create_reader_task ( p_io_scheduler_t scheduler,
                              fd_t fd, int64_t time_out, void * user_data, io_scheduler_cbk_t read_cbk )
{
  io_task_opts_t opts = IO_SCHEDULER_READ;
  if ( time_out > 0 )
    opts |= IO_SCHEDULER_TIMER;
  LOGSVC_TRACE( "io_sched_create_reader_task(): fd == %d, time_out == %lu", fd, time_out );
  return io_sched_create_task ( scheduler, fd, opts, time_out, user_data,
                                read_cbk, NIL_IO_SCHEDULER_CBK, NIL_IO_SCHEDULER_CBK, read_cbk );
}

/**
 * Creates a read-only task for the given file descriptor, plus will check for OOB data; waits
 * up to time_out microseconds before calling the read callback with a timeout error.
 **/
p_io_scheduler_task_t
io_sched_create_reader_task_ex ( p_io_scheduler_t scheduler,
                                 fd_t fd, int64_t time_out, void * user_data, 
                                 io_scheduler_cbk_t read_cbk, io_scheduler_cbk_t err_cbk )
{
  io_task_opts_t opts = IO_SCHEDULER_READ | IO_SCHEDULER_ERROR;
  if ( time_out > 0 )
    opts |= IO_SCHEDULER_TIMER;
  LOGSVC_TRACE( "io_sched_create_reader_task_ex(): fd == %d, time_out == %lu", fd, time_out );
  return io_sched_create_task ( scheduler, fd, opts, time_out, user_data,
                                read_cbk, NIL_IO_SCHEDULER_CBK, err_cbk, read_cbk );
}

/**
 * Creates a scheduler and tells it how many tasks it can handle concurrently.
 * @param max_concurrent_tasks Max number of tasks scheduler can have scheduled at any given time.
 * @param max_num_timers Max number of timers scheduler can have scheduled at any given time.
 * @return The IO scheduler, or NULL if unable to create the scheduler.
 **/
p_io_scheduler_t
io_sched_create_scheduler ( size_t max_concurrent_tasks, size_t max_num_timers )
{
  p_io_scheduler_t rv;
  size_t ii;
  fd_t timer_id;
  
  LOGSVC_TRACE( "io_sched_create_scheduler(): max_tasks == %d, max_timers == %d", max_concurrent_tasks, max_num_timers );
  
  if ( !(max_concurrent_tasks) || !(max_num_timers) )
    return NIL_IO_SCHEDULER;
  
  rv = (p_io_scheduler_t) malloc ( IO_SCHEDULER_STRUCT_SIZE );
  if ( rv ) {
    memset ( rv, 0, IO_SCHEDULER_STRUCT_SIZE );
    rv->task_pool = stack_fixed_init ( max_concurrent_tasks );
    rv->timer_id_pool = stack_fixed_init ( max_num_timers );
    if ( !(rv->task_pool) || !(rv->timer_id_pool) ) {
      io_sched_destroy_scheduler ( rv );
      return NIL_IO_SCHEDULER;
    }
    /* Create the tasks */
    for ( ii=0; ii < max_concurrent_tasks; ii++ ) {
      void * tmp = malloc ( IO_SCHEDULER_TASK_STRUCT_SIZE );
      if ( !(tmp) ) {
        fprintf ( stderr, "CRITICAL: io_sched_create_scheduler() : Out of memory.\n" );
        io_sched_destroy_scheduler ( rv );
        return NIL_IO_SCHEDULER;
      }
      if ( stack_fixed_push ( rv->task_pool, tmp ) == STACK_ERROR_FULL )
        break;
    }
    
    /* Create the timers */
    for ( timer_id = -max_num_timers - 2; timer_id < -2; timer_id++ ) {
      if ( stack_fixed_push ( rv->timer_id_pool, (void*) timer_id ) == STACK_ERROR_FULL )
        break;
    }
    
    pthread_mutex_init ( &(rv->task_list_mutex), (const pthread_mutexattr_t *) 0 );
    pthread_mutex_init ( &(rv->task_pool_mutex), (const pthread_mutexattr_t *) 0 );
    pthread_mutex_init ( &(rv->timer_pool_mutex), (const pthread_mutexattr_t *) 0 );
  }
  return rv;
}

/**
 * Creates a task to be added to our IO scheduler; may specify the file descriptor, timeout length,
 * read/write/error/timeout callbacks.
 **/
p_io_scheduler_task_t
io_sched_create_task ( p_io_scheduler_t scheduler,
                       fd_t fd, io_task_opts_t opts, int64_t time_out, void * user_data,
                       io_scheduler_cbk_t read_cbk,
                       io_scheduler_cbk_t write_cbk,
                       io_scheduler_cbk_t err_cbk,
                       io_scheduler_cbk_t time_out_cbk )
{
  p_io_scheduler_task_t ptask;
  
  if ( !(scheduler) )
    return NIL_IO_SCHEDULER_TASK;
  
  LOGSVC_TRACE( "io_sched_create_task(): fd == %d, opts == %u, time_out == %lu", fd, opts, time_out );
  ptask = (p_io_scheduler_task_t) stack_fixed_pop_and_return_r ( scheduler->task_pool_mutex, scheduler->task_pool );
  if ( ptask ) {
    memset ( ptask, 0, IO_SCHEDULER_TASK_STRUCT_SIZE );
    if ( opts & IO_SCHEDULER_REMOVE )
      opts &= ~IO_SCHEDULER_REMOVE;
    ptask->owner = scheduler;
    ptask->fd = fd;
    ptask->opts = opts;
    ptask->time_out = time_out;
    ptask->user_data = user_data;
    ptask->on_read_rdy_cbk = read_cbk;
    ptask->on_write_rdy_cbk = write_cbk;
    ptask->on_err_rdy_cbk = err_cbk;
    ptask->on_timeout_cbk = time_out_cbk;
  }
  return ptask;
}

/**
 * Creates a task that will execute after the specified amount of time; no file descriptor is provided.
 **/
p_io_scheduler_task_t
io_sched_create_timer_task ( p_io_scheduler_t scheduler,
                             int64_t time_out, void * user_data, io_scheduler_cbk_t time_out_cbk )
{
  fd_t timer_id;
  
  if ( !(scheduler) )
    return NIL_IO_SCHEDULER_TASK;
  
  timer_id = (fd_t) stack_fixed_pop_and_return_r ( scheduler->timer_pool_mutex, scheduler->timer_id_pool );
  if ( !(timer_id) )
    return NIL_IO_SCHEDULER_TASK;
  
  LOGSVC_TRACE( "io_sched_create_timer_task(): timer_id == %d, time_out == %lu", timer_id, time_out );
  return io_sched_create_task ( scheduler, timer_id, IO_SCHEDULER_TIMER, time_out, user_data,
                                NIL_IO_SCHEDULER_CBK, NIL_IO_SCHEDULER_CBK, NIL_IO_SCHEDULER_CBK, time_out_cbk );
}

/**
 * Creates a write-only task for the given file descriptor; will wait only time_out microseconds
 * before calling the write callback method with a timeout error.
 **/
p_io_scheduler_task_t
io_sched_create_writer_task ( p_io_scheduler_t scheduler,
                              fd_t fd, int64_t time_out, void * user_data, io_scheduler_cbk_t write_cbk )
{
  io_task_opts_t opts = IO_SCHEDULER_WRITE;
  if ( time_out > 0 )
    opts |= IO_SCHEDULER_TIMER;
  LOGSVC_TRACE( "io_sched_create_writer_task(): fd == %d, time_out == %lu", fd, time_out );
  return io_sched_create_task ( scheduler, fd, opts, time_out, user_data,
                                NIL_IO_SCHEDULER_CBK, write_cbk, NIL_IO_SCHEDULER_CBK, write_cbk );
}

/**
 * Destroys a scheduler, releasing any tasks/timers therein.
 **/
void
io_sched_destroy_scheduler ( p_io_scheduler_t scheduler )
{
  p_io_scheduler_task_t ptask;
  
  LOGSVC_DEBUG( "io_sched_destroy_scheduler()" );
  
  if ( scheduler ) {
    /* Clear any scheduled tasks */
    LOGSVC_DEBUG( "Clearing scheduled tasks" );
    LOCK_MUTEX( scheduler->task_list_mutex );
    while ( scheduler->scheduled_tasks ) {
      ptask = scheduler->scheduled_tasks->next;
      io_sched_destroy_task ( scheduler->scheduled_tasks );
      scheduler->scheduled_tasks = ptask;
    }
    UNLOCK_MUTEX( scheduler->task_list_mutex );
    
    /* Remove any tasks in the task pool. */
    LOGSVC_DEBUG( "Clearing task pool" );
    while ( (ptask = (p_io_scheduler_task_t) stack_fixed_pop_and_return ( scheduler->task_pool )) != NIL_IO_SCHEDULER_TASK ) {
      free ( ptask );
    }
    LOGSVC_DEBUG( "Destroying task pool" );
    stack_fixed_destroy ( scheduler->task_pool );
    
    /* Remove the timer pool. */
    LOGSVC_DEBUG( "Destroying timer pool" );
    stack_fixed_destroy ( scheduler->timer_id_pool );
    
    /* Free the mutexes */
    pthread_mutex_destroy ( &(scheduler->timer_pool_mutex) );
    pthread_mutex_destroy ( &(scheduler->task_pool_mutex) );
    pthread_mutex_destroy ( &(scheduler->task_list_mutex) );
    
    free ( scheduler );
    
    LOGSVC_DEBUG( "Scheduler destroyed" );
  }
  
}

/**
 * Locates a task in the scheduler based on its file descriptor / timer ID.
 **/
p_io_scheduler_task_t
io_sched_find_task ( p_io_scheduler_t scheduler, fd_t fd )
{
  p_io_scheduler_task_t ptask = NIL_IO_SCHEDULER_TASK;
  LOGSVC_TRACE( "io_sched_find_task(): fd == %d", fd );
  if ( scheduler ) {
    LOCK_MUTEX( scheduler->task_list_mutex );
    ptask = scheduler->scheduled_tasks;
    while ( ptask && ( ptask->fd != fd ) )
      ptask = ptask->next;
    UNLOCK_MUTEX( scheduler->task_list_mutex );
  }
  return ptask;
}

/**
 * Reschedules a task, essentially updating its expiration time.
 **/
bool_t
io_sched_reschedule_task ( p_io_scheduler_task_t io_task )
{
  if ( !( io_task ) )
    return ICP_FALSE;
  if ( S_IOSCHED_OPTS_REMOVE( io_task ) )
    return ICP_FALSE;
  LOGSVC_TRACE( "io_sched_reschedule_task(): Rescheduling task FD == %d", io_task->fd );
  inl_io_sched_populate_expire_time ( io_task );
  return ICP_TRUE;
}

/**
 * Runs the specified scheduler in the current thread of execution.
 **/
void
io_sched_run_scheduler ( p_io_scheduler_t scheduler )
{
  LOGSVC_DEBUG( "io_sched_run_scheduler()" );
  
  if ( !(scheduler) )
    return;
  
  while ( !( scheduler->stop_scheduler ) && ( scheduler->scheduled_tasks ) ) {
    inl_io_sched_pump ( scheduler );
  }
  
}

/**
 * Adds a task to the IO scheduler.
 **/
bool_t
io_sched_schedule_task ( p_io_scheduler_task_t io_task )
{
  if ( !io_task )
    return ICP_FALSE;
  
  if ( S_IOSCHED_OPTS_READ( io_task ) && !(io_task->on_read_rdy_cbk) )
    io_task->opts &= ~IO_SCHEDULER_READ;
  if ( S_IOSCHED_OPTS_WRITE( io_task ) && !(io_task->on_write_rdy_cbk) )
    io_task->opts &= ~IO_SCHEDULER_WRITE;
  if ( S_IOSCHED_OPTS_ERROR( io_task ) && !(io_task->on_err_rdy_cbk) )
    io_task->opts &= ~IO_SCHEDULER_ERROR;
  if ( S_IOSCHED_OPTS_TIMER( io_task ) &&
       ( !(io_task->on_timeout_cbk) || ( io_task->time_out == IO_SCHEDULER_NO_TIMEOUT ) ) )
    io_task->opts &= ~IO_SCHEDULER_TIMER;
  if ( io_task->opts == IO_SCHEDULER_NONE )
    io_task->opts |= IO_SCHEDULER_REMOVE;
  
  if ( S_IOSCHED_OPTS_TIMER( io_task ) ) {
    inl_io_sched_populate_expire_time ( io_task );
  }
  
  LOCK_MUTEX( io_task->owner->task_list_mutex );
  do {
    p_io_scheduler_task_t ttt = io_task->owner->scheduled_tasks;
    if ( !ttt )
      io_task->owner->scheduled_tasks = io_task;
    else {
      while ( ttt->next )
        ttt = ttt->next;
      ttt->next = io_task;
    }
    //io_task->next = io_task->owner->scheduled_tasks;
    //io_task->owner->scheduled_tasks = io_task;
  } while ( 0 );
  
  if ( ICP_DEBUG_ENABLED ) {
    p_io_scheduler_task_t t = io_task->owner->scheduled_tasks;
    int iii = 0;
    while ( t ) {
      iii++;
      t = t->next;
    }
    LOGSVC_DEBUG( "Scheduler has %d tasks scheduled.", iii );
  }
  
  UNLOCK_MUTEX( io_task->owner->task_list_mutex );
  
  return ICP_TRUE;
}

/**
 * Runs the specified scheduler in a secondary thread of execution.
 * @return True if able to successfully start the scheduler; otherwise, false.
 **/
bool_t
io_sched_start_scheduler_thread ( p_io_scheduler_t scheduler )
{
  int rc;
  int tries_left = 3;
  
  LOGSVC_DEBUG( "io_sched_start_scheduler_thread()" );
  
  if ( !(scheduler) || ( scheduler->stop_scheduler ) )
    return ICP_FALSE;
  
  do
  {
    rc = pthread_create ( &(scheduler->scheduler_thread),
                          (const pthread_attr_t *) 0,
                          io_sched_threadfn, scheduler );
    if ( rc == 0 )
      return ICP_TRUE;
    else if ( rc == EAGAIN )
      usleep ( IO_SCHEDULER_UTIME_QTR_SECOND );
  }
  while ( ( rc == EAGAIN ) && ( --tries_left > 0 ) );
  
  if ( rc == EAGAIN )
    LOGSVC_ERROR( "io_sched_start_scheduler_thread(): Too many threads; cannot create another." );
  return ICP_FALSE;
}

/**
 * Tells the specified scheduler it should stop processing its tasks.
 **/
void
io_sched_stop_scheduler ( p_io_scheduler_t scheduler )
{
  p_io_scheduler_task_t task;
  
  LOGSVC_DEBUG( "io_sched_stop_scheduler()" );
  if ( scheduler ) {
    
    // Clear out the scheduled tasks.
    LOCK_MUTEX( scheduler->task_list_mutex );
    task = scheduler->scheduled_tasks;
    while ( task ) {
      task->opts |= IO_SCHEDULER_REMOVE;
      task = task->next;
    }
    scheduler->stop_scheduler = ICP_TRUE;
    UNLOCK_MUTEX( scheduler->task_list_mutex );
    
    if ( scheduler->scheduler_thread ) {
      //LOGSVC_DEBUG( "Killing scheduler thread" );
      //pthread_kill ( scheduler->scheduler_thread, SIGKILL );
      //LOGSVC_DEBUG( "Detaching scheduler thread" );
      //pthread_detach ( scheduler->scheduler_thread );
      LOGSVC_DEBUG( "Cancelling scheduler thread" );
      if ( pthread_cancel ( scheduler->scheduler_thread ) == 0 ) {
        LOGSVC_DEBUG( "Scheduler thread cancelled" );
        pthread_join ( scheduler->scheduler_thread, NULL );
        LOGSVC_DEBUG( "Scheduler thread joined" );
      }
    }
    //else {
    //  LOGSVC_DEBUG( "Setting scheduler stop flag" );
    //  scheduler->stop_scheduler = ICP_TRUE;
    //}
  }
}

/**
 * Tells the IO scheduler to remove the task the next time through its loop.
 **/
void
io_sched_unschedule_task ( p_io_scheduler_task_t io_task )
{
  LOGSVC_DEBUG( "io_sched_unschedule_task(): FD == %d", io_task->fd );
  if ( io_task && ( io_task->fd != INVALID_GENERAL_FD ) )
    io_task->opts |= IO_SCHEDULER_REMOVE;
}

/* ---------- ---------- ---------- ---------- */

/**
 * Tries to process a scheduled task according to the options therein and whether or not its file
 * descriptor exists in one of the read/write/error FD sets.
 * @return True if task was completed and should be unscheduled; false if task was not completed and needs to remain scheduled.
 **/
bool_t
io_sched_process_task ( p_io_scheduler_task_t io_task, fd_set * rds, fd_set * wrs, fd_set * ers )
{
  bool_t rv = ICP_TRUE;
  struct timespec ts_now;
  bool_t task_expired;
  
  //LOGSVC_TRACE( "io_sched_process_task(): FD == %d", io_task->fd );
  
  clock_gettime ( CLOCK_REALTIME, &ts_now );
  task_expired = ( ( io_task->time_out != IO_SCHEDULER_NO_TIMEOUT ) &&
                   ( ( ts_now.tv_sec > io_task->expire_time.tv_sec ) ||
                     ( ( ts_now.tv_sec == io_task->expire_time.tv_sec ) &&
                       ( ts_now.tv_nsec >= io_task->expire_time.tv_nsec ) ) ) );
  
  if ( io_task->fd > INVALID_GENERAL_FD ) {
    /* IO */
    
    /* Check for error-ready (MOB) */
    if ( S_IOSCHED_OPTS_ERROR( io_task ) && FD_ISSET( io_task->fd, ers ) ) {
      io_task->on_err_rdy_cbk ( io_task, IO_SCHEDULER_ERR_NONE );
      FD_CLR ( io_task->fd, ers );
    }
    
    /* Check for read-ready */
    if ( S_IOSCHED_OPTS_READ( io_task ) ) {
      if ( FD_ISSET( io_task->fd, rds ) ) {
        rv = rv && io_task->on_read_rdy_cbk ( io_task, IO_SCHEDULER_ERR_NONE );
        FD_CLR ( io_task->fd, rds );
      }
      else if ( task_expired ) {
        rv = rv && io_task->on_timeout_cbk ( io_task, IO_SCHEDULER_ERR_OP_TIMEOUT );
      }
      else {
        rv = ICP_FALSE;
      }
    }
    
    /* Check for write-ready */
    if ( S_IOSCHED_OPTS_WRITE( io_task ) ) {
      if ( FD_ISSET( io_task->fd, wrs ) ) {
        rv = rv && io_task->on_write_rdy_cbk ( io_task, IO_SCHEDULER_ERR_NONE );
        FD_CLR ( io_task->fd, wrs );
      }
      else if ( task_expired ) {
        rv = rv && io_task->on_timeout_cbk ( io_task, IO_SCHEDULER_ERR_OP_TIMEOUT );
      }
      else {
        rv = ICP_FALSE;
      }
    }
  }
  else {
    /* Timer */
    
    if ( task_expired ) {
      if ( !( io_task->on_timeout_cbk ( io_task, IO_SCHEDULER_ERR_OP_TIMEOUT ) ) ) {
        /* Wants to repeat after the original timeout amount of time again */
        inl_io_sched_populate_expire_time ( io_task );
        rv = ICP_FALSE;
      }
    }
    else {
      rv = ICP_FALSE;
    }
  }
  
  if ( io_task->fd < 0 )
    LOGSVC_TRACE( "io_sched_process_task(): FD == %d ; rv == %d", io_task->fd, rv );
  
  return rv;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

/**
 * Destroys an IO scheduler task; user data is not destroyed, nor is the file descriptor closed -
 * those are owned by the calling process.
 **/
static void
io_sched_destroy_task ( p_io_scheduler_task_t io_task )
{
  if ( io_task ) {
    LOGSVC_TRACE( "io_sched_destroy_task(): fd == %d", io_task->fd );
    if ( io_task->fd < INVALID_GENERAL_FD )
      stack_fixed_push_r ( io_task->owner->timer_pool_mutex, io_task->owner->timer_id_pool, (void*) io_task->fd );
    if ( stack_fixed_push_r ( io_task->owner->task_pool_mutex, io_task->owner->task_pool, io_task ) == STACK_ERROR_FULL )
      free ( io_task );
    else
      memset ( io_task, 0, IO_SCHEDULER_TASK_STRUCT_SIZE );
  }
}

/**
 * Thread entry point for schedulers that run in secondary process threads.
 **/
static void *
io_sched_threadfn ( void * ud )
{
  p_io_scheduler_t scheduler = (p_io_scheduler_t) ud;
  
  LOGSVC_DEBUG( "io_sched_threadfn()" );
  
  if ( scheduler ) {
    while ( !( scheduler->stop_scheduler ) ) {
      inl_io_sched_pump ( scheduler );
    }
    LOGSVC_DEBUG( "io_sched_threadfn(): scheduler->stop_scheduler set to true" );
  }
  
  return ud; // Don't really need to return anything
}

/**
 * Inline helper function that calculates the expiry time for a task that has a timeout associated with it.
 **/
static inline void
inl_io_sched_populate_expire_time ( p_io_scheduler_task_t io_task )
{
  int64_t time_to_add = io_task->time_out;
  struct timespec * pts = &( io_task->expire_time );
  
  LOGSVC_TRACE( "inl_io_sched_populate_expire_time(): Timeout: %lu", time_to_add );
  
  clock_gettime ( CLOCK_REALTIME, &(io_task->expire_time) );
  LOGSVC_TRACE( "inl_io_sched_populate_expire_time(): Current clock: %lu.%lu", pts->tv_sec, pts->tv_nsec );
  while ( time_to_add > IO_SCHEDULER_TIME_ONE_SECOND ) {
    pts->tv_sec++;
    time_to_add -= IO_SCHEDULER_TIME_ONE_SECOND;
  }
  assert ( time_to_add >= 0 );
  pts->tv_nsec += time_to_add;
  if ( pts->tv_nsec > IO_SCHEDULER_TIME_ONE_SECOND ) {
    pts->tv_sec++;
    pts->tv_nsec -= IO_SCHEDULER_TIME_ONE_SECOND;
  }
  LOGSVC_TRACE( "inl_io_sched_populate_expire_time(): Expire time: %lu.%lu", pts->tv_sec, pts->tv_nsec );
}

static inline void
inl_io_sched_pump ( p_io_scheduler_t scheduler )
{
  fd_set rd, wr, er;
  fd_t maxfd = INVALID_GENERAL_FD; /* -1 */
  p_io_scheduler_task_t ptask, pnext_task;
  struct timeval tv_select_timeout;
  
  if ( !( scheduler->scheduled_tasks ) && ( scheduler->scheduler_thread ) ) {
    /* Nothing to do, and we're running in a secondary thread; give the system a chance to do something else. */
    memset ( &tv_select_timeout, 0, sizeof( struct timeval ) );
    tv_select_timeout.tv_usec = 1000; /* 1 ms */
    select ( 0, NULL, NULL, NULL, &tv_select_timeout );
    return;
  }
  
  FD_ZERO ( &rd );
  FD_ZERO ( &wr );
  FD_ZERO ( &er );
  
  /*
   *
   * We lock a mutex here, specifically so that we do not break any links while someone else
   * is possibly trying to add to our task list. We don't have to worry about that later; this
   * is the only place where tasks are removed from the list - everyone else merely requests
   * their removal. When we loop later to check for the file descriptors being in the FD sets
   * it will not matter if a new file descriptor was added into our task list, simply because
   * there is no way for it to be set in the FD sets since we were never checking for it.
   *
   * When removing, we do not allow the tasks with an FD of INVALID_FD to be removed. These are
   * special, recurring system tasks that need to remain scheduled at all times.
   *
   */
  if ( scheduler->scheduled_tasks && !( scheduler->stop_scheduler ) ) {
    LOCK_MUTEX( scheduler->task_list_mutex );
    
    /* Clear any removes from the front of the scheduled task list. */
    while ( scheduler->scheduled_tasks &&
            S_IOSCHED_OPTS_REMOVE( scheduler->scheduled_tasks ) &&
            ( scheduler->scheduled_tasks->fd != INVALID_GENERAL_FD ) )
    {
      ptask = scheduler->scheduled_tasks->next;
      io_sched_destroy_task ( scheduler->scheduled_tasks );
      scheduler->scheduled_tasks = ptask;
    }
    
    /* At this point we know that either (a) the list is empty, or (b) the first item is ready to be processed. */
    ptask = scheduler->scheduled_tasks;
    while ( ptask ) {
      
      /* If it is a timer task, we don't need to worry about the FD sets. */
      if ( !(S_IOSCHED_OPTS_TIMER_ONLY( ptask )) ) {
        if ( S_IOSCHED_OPTS_READ( ptask ) ) {
          maxfd = ( ptask->fd > maxfd ) ? ptask->fd : maxfd;
          FD_SET ( ptask->fd, &rd );
        }
        if ( S_IOSCHED_OPTS_WRITE( ptask ) ) {
          maxfd = ( ptask->fd > maxfd ) ? ptask->fd : maxfd;
          FD_SET ( ptask->fd, &wr );
        }
        if ( S_IOSCHED_OPTS_ERROR( ptask ) ) {
          maxfd = ( ptask->fd > maxfd ) ? ptask->fd : maxfd;
          FD_SET ( ptask->fd, &er );
        }
      }
      
      /* Scan ahead for removes ... */
      while ( ptask->next && S_IOSCHED_OPTS_REMOVE( ptask->next ) && ( ptask->next->fd != INVALID_GENERAL_FD ) ) {
        pnext_task = ptask->next->next;
        io_sched_destroy_task ( ptask->next );
        ptask->next = pnext_task;
      }
      
      ptask = ptask->next;
    }
    
    UNLOCK_MUTEX( scheduler->task_list_mutex );
  }
  
  if ( !( scheduler->stop_scheduler ) ) {
    memset ( &tv_select_timeout, 0, sizeof( struct timeval ) );
    tv_select_timeout.tv_usec = 10000; /* 10 ms */
    if ( select ( maxfd + 1, &rd, &wr, &er, &tv_select_timeout ) < 0 )
      return;
    
    ptask = scheduler->scheduled_tasks;
    while ( ptask && !( scheduler->stop_scheduler ) ) {
      if ( !S_IOSCHED_OPTS_REMOVE( ptask ) ) {
        if ( io_sched_process_task ( ptask, &rd, &wr, &er ) )
          io_sched_unschedule_task ( ptask );
      }
      ptask = ptask->next;
    }
  }
  
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
