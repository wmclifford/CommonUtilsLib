/**
 * @file    child-process.c
 * @author  William Clifford
 *
 **/

#include "child-process.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Constants  */
/* ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Type definitions and structures  */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Shared (global) variables        */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local function prototypes        */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Module variables      */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

void
child_proc_destroy ( p_child_proc_t child )
{
  if ( child ) {
    // Not much to do here ... simply free() the memory held by the structure instance.
    free ( child );
  }
}

p_child_proc_t
child_proc_init ( pid_t pid, fd_t fd, void * userdata )
{
  p_child_proc_t rv = NEW_child_proc();
  if ( rv ) {
    rv->pid = pid;
    rv->fd = fd;
    rv->user_data = userdata;
    rv->on_exit = (child_proc_exited_t) 0;
  }
  return rv;
}

p_child_proc_t
child_proc_init_full ( pid_t pid, fd_t fd, void * userdata, child_proc_exited_t on_exit_cbk )
{
  p_child_proc_t rv = NEW_child_proc();
  if ( rv ) {
    rv->pid = pid;
    rv->fd = fd;
    rv->user_data = userdata;
    rv->on_exit = on_exit_cbk;
  }
  return rv;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
