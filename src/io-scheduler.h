/**
 * @file    io-scheduler.h
 * @author  William Clifford
 * 
 **/

#ifndef IO_SCHEDULER_H__
#define IO_SCHEDULER_H__

/* Include the precompiled header for all the standard library includes and project-wide
   definitions. */
#include "gccpch.h"

#include "stack.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

// Forward declarations for use in the scheduler task structure.

struct _io_scheduler;
struct _io_scheduler_task;

typedef enum {
  IO_SCHEDULER_NONE                       = 0x00000000,
    IO_SCHEDULER_READ                     = 0x00000001,
    IO_SCHEDULER_WRITE                    = 0x00000002,
    IO_SCHEDULER_ERROR                    = 0x00000004,
    IO_SCHEDULER_TIMER                    = 0x00000008,
    IO_SCHEDULER_REMOVE                   = 0x80000000
} io_task_opts_t;

#define IO_SCHEDULER_TASK_COMPLETE        ICP_TRUE
#define IO_SCHEDULER_TASK_INCOMPLETE      ICP_FALSE

/**
 * Callback function used for handling reads/writes/errors and timeouts. Function will
 * receive the scheduler handling the IO/timer, the file descriptor / timer ID, an
 * error code indicating whether or not the callback was called due to an error condition,
 * and an application-specific value that was given upon creation of the scheduled task.
 * 
 * If the function returns True, the scheduler considers the task to be complete, and will
 * remove it from its scheduled tasks list. If the function returns False, the scheduler
 * treats this as the task has not completed and should remain in the scheduled tasks
 * list for further processing.
 * 
 **/
typedef bool_t ( *io_scheduler_cbk_t ) ( struct _io_scheduler_task * task, int errcode );
//typedef bool_t ( *io_scheduler_cbk_t ) ( struct _io_scheduler * scheduler, fd_t fd, int errcode, void * userdata );

/* ---------- ---------- ---------- ---------- */

typedef struct _io_scheduler_task {
  
  struct _io_scheduler_task * next;
  struct _io_scheduler_task * prev;
  struct _io_scheduler * owner;
  fd_t fd;
  volatile io_task_opts_t opts;
  int64_t time_out;
  struct timespec time_scheduled;
  struct timespec expire_time;
  void * user_data;
  io_scheduler_cbk_t on_read_rdy_cbk;
  io_scheduler_cbk_t on_write_rdy_cbk;
  io_scheduler_cbk_t on_err_rdy_cbk;
  io_scheduler_cbk_t on_timeout_cbk;
  
} io_scheduler_task_t;

typedef struct _io_scheduler_task * p_io_scheduler_task_t;

#define IO_SCHEDULER_TASK_STRUCT_SIZE   (sizeof( struct _io_scheduler_task ))

/* ---------- ---------- ---------- ---------- */

typedef struct _io_scheduler {
  
  /** Linked list of currently scheduled tasks. */
  p_io_scheduler_task_t scheduled_tasks;
  
  /** Mutex used to lock the task list. */
  pthread_mutex_t task_list_mutex;
  
  /** A pool of tasks; rather than malloc/free a lot of times, make a bunch up front. */
  p_stack_t task_pool;
  pthread_mutex_t task_pool_mutex;
  
  /** A pool of timer IDs for non-IO tasks. */
  p_stack_t timer_id_pool;
  pthread_mutex_t timer_pool_mutex;
  
  /**
   * Optional thread in which the scheduler runs.
   * May run in main thread, in which case this will remain NULL.
   **/
  pthread_t scheduler_thread;
  
  /** Flag indicating whether scheduler should stop. */
  volatile bool_t stop_scheduler;
  
} io_scheduler_t;

typedef struct _io_scheduler * p_io_scheduler_t;

#define IO_SCHEDULER_STRUCT_SIZE        (sizeof( struct _io_scheduler ))

/* ---------- ---------- ---------- ---------- */

/* "NULL" indicators for pointers to the aforementioned structures. */
#define NIL_IO_SCHEDULER                ((p_io_scheduler_t) 0)
#define NIL_IO_SCHEDULER_CBK            ((io_scheduler_cbk_t) 0)
#define NIL_IO_SCHEDULER_TASK           ((p_io_scheduler_task_t) 0)

// Tells scheduler that task never times out.
#define IO_SCHEDULER_NO_TIMEOUT         ((int64_t) -1)

// We default to using microseconds; in the event we need to switch to nanoseconds, both are defined here.
#define IO_SCHEDULER_NTIME_ONE_SECOND   1000000000
#define IO_SCHEDULER_UTIME_ONE_SECOND   1000000
#define IO_SCHEDULER_UTIME_HALF_SECOND  500000
#define IO_SCHEDULER_UTIME_QTR_SECOND   250000
#define IO_SCHEDULER_TIME_ONE_SECOND    IO_SCHEDULER_NTIME_ONE_SECOND

// Scheduler error codes.
#define IO_SCHEDULER_ERR_NONE           0x00000000
#define IO_SCHEDULER_ERR_BAD_FD         EBADFD
#define IO_SCHEDULER_ERR_WOULDBLOCK     EWOULDBLOCK
#define IO_SCHEDULER_ERR_OP_TIMEOUT     ETIME
#define IO_SCHEDULER_ERR_FD_CLOSED      ECONNRESET
#define IO_SCHEDULER_ERR_FD_EOF         ENODATA

#define S_IOSCHED_OPTS_READ(t)          ((t)->opts & IO_SCHEDULER_READ)
#define S_IOSCHED_OPTS_WRITE(t)         ((t)->opts & IO_SCHEDULER_WRITE)
#define S_IOSCHED_OPTS_ERROR(t)         ((t)->opts & IO_SCHEDULER_ERROR)
#define S_IOSCHED_OPTS_TIMER(t)         ((t)->opts & IO_SCHEDULER_TIMER)
#define S_IOSCHED_OPTS_TIMER_ONLY(t)    ((t)->opts == IO_SCHEDULER_TIMER)
#define S_IOSCHED_OPTS_REMOVE(t)        ((t)->opts & IO_SCHEDULER_REMOVE)

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

/**
 * Creates a read-only task for the given file descriptor; will wait only time_out microseconds
 * before calling the read callback method with a timeout error.
 **/
p_io_scheduler_task_t io_sched_create_reader_task ( p_io_scheduler_t scheduler,
                                                    fd_t fd, int64_t time_out, void * user_data,
                                                    io_scheduler_cbk_t read_cbk );

/**
 * Creates a read-only task for the given file descriptor, plus will check for OOB data; waits
 * up to time_out microseconds before calling the read callback with a timeout error.
 **/
p_io_scheduler_task_t io_sched_create_reader_task_ex ( p_io_scheduler_t scheduler,
                                                       fd_t fd, int64_t time_out, void * user_data,
                                                       io_scheduler_cbk_t read_cbk,
                                                       io_scheduler_cbk_t err_cbk );

/**
 * Creates a scheduler and tells it how many tasks it can handle concurrently.
 * @param max_concurrent_tasks Max number of tasks scheduler can have scheduled at any given time.
 * @param max_num_timers Max number of timers scheduler can have scheduled at any given time.
 * @return The IO scheduler, or NULL if unable to create the scheduler.
 **/
p_io_scheduler_t io_sched_create_scheduler ( size_t max_concurrent_tasks, size_t max_num_timers );

/**
 * Creates a task to be added to our IO scheduler; may specify the file descriptor, timeout length,
 * read/write/error/timeout callbacks.
 **/
p_io_scheduler_task_t io_sched_create_task ( p_io_scheduler_t scheduler,
                                             fd_t fd,
                                             io_task_opts_t opts,
                                             int64_t time_out,
                                             void * user_data,
                                             io_scheduler_cbk_t read_cbk,
                                             io_scheduler_cbk_t write_cbk,
                                             io_scheduler_cbk_t err_cbk,
                                             io_scheduler_cbk_t time_out_cbk );

/**
 * Creates a task that will execute after the specified amount of time; no file descriptor is provided.
 **/
p_io_scheduler_task_t io_sched_create_timer_task ( p_io_scheduler_t scheduler,
                                                   int64_t time_out, void * user_data,
                                                   io_scheduler_cbk_t time_out_cbk );

/**
 * Creates a write-only task for the given file descriptor; will wait only time_out microseconds
 * before calling the write callback method with a timeout error.
 **/
p_io_scheduler_task_t io_sched_create_writer_task ( p_io_scheduler_t scheduler,
                                                    fd_t fd, int64_t time_out, void * user_data,
                                                    io_scheduler_cbk_t write_cbk );

/**
 * Destroys a scheduler, releasing any tasks/timers therein.
 **/
void io_sched_destroy_scheduler ( p_io_scheduler_t scheduler );

/**
 * Locates a task in the scheduler based on its file descriptor / timer ID.
 **/
p_io_scheduler_task_t io_sched_find_task ( p_io_scheduler_t scheduler, fd_t fd );

/**
 * Reschedules a task, essentially updating its expiration time.
 **/
bool_t io_sched_reschedule_task ( p_io_scheduler_task_t io_task );

/**
 * Runs the specified scheduler in the current thread of execution.
 **/
void io_sched_run_scheduler ( p_io_scheduler_t scheduler );

/**
 * Adds a task to the IO scheduler.
 **/
bool_t io_sched_schedule_task ( p_io_scheduler_task_t io_task );

/**
 * Runs the specified scheduler in a secondary thread of execution.
 * @return True if able to successfully start the scheduler; otherwise, false.
 **/
bool_t io_sched_start_scheduler_thread ( p_io_scheduler_t scheduler );

/**
 * Tells the specified scheduler it should stop processing its tasks.
 **/
void io_sched_stop_scheduler ( p_io_scheduler_t scheduler );

/**
 * Tells the IO scheduler to remove the task the next time through its loop.
 **/
void io_sched_unschedule_task ( p_io_scheduler_task_t io_task );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* IO_SCHEDULER_H__ */
