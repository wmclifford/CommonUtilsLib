/**
 * @file    child-process.h
 * @author  William Clifford
 **/

#ifndef CHILD_PROCESS_H__
#define CHILD_PROCESS_H__

/* Include the precompiled header for all the standard library includes and project-wide definitions. */
#include "gccpch.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Constants  */
/* ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Type definitions and structures  */
/* ---------- ---------- ---------- */

struct _child_proc;

typedef void ( *child_proc_exited_t ) ( struct _child_proc * child, int exit_status );

typedef struct _child_proc {

  pid_t                           pid;
  fd_t                            fd;   // Left as generic FD type so that it can be a pipe, socket, etc.
  void *                          user_data;
  
  child_proc_exited_t             on_exit;

} child_proc_t, * p_child_proc_t;

#define SIZE_child_proc                 (sizeof( struct _child_proc ))
#define NEW_child_proc()                ( (p_child_proc_t) malloc ( sizeof( struct _child_proc ) ) )
#define NIL_child_proc                  ( (p_child_proc_t) 0 )
#define AS_PTR_child_proc(vp)           ( (p_child_proc_t) vp )

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

void child_proc_destroy ( p_child_proc_t child );

p_child_proc_t child_proc_init ( pid_t pid, fd_t fd, void * userdata );

p_child_proc_t child_proc_init_full ( pid_t pid, fd_t fd, void * userdata, child_proc_exited_t on_exit_cbk );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* CHILD_PROCESS_H__ */
