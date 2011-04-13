/**
 * @file    child-process-mgr.h
 * @author  William Clifford
 **/

#ifndef CHILD_PROCESS_MGR_H__
#define CHILD_PROCESS_MGR_H__

/* Include the precompiled header for all the standard library includes and project-wide definitions. */
#include "gccpch.h"

#include "child-process.h"
#include "io-scheduler.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Constants  */
/* ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Type definitions and structures  */
/* ---------- ---------- ---------- */

struct _child_proc_mgr;
typedef struct _child_proc_mgr * p_child_proc_mgr_t;

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

void child_proc_mgr_destroy ( struct _child_proc_mgr * cpmgr );

struct _child_proc_mgr * child_proc_mgr_init ( void );

bool_t child_proc_mgr_monitor_child ( struct _child_proc_mgr * cpmgr, p_child_proc_t child );

bool_t child_proc_mgr_monitor_pid ( struct _child_proc_mgr * cpmgr,
                                    pid_t pid, fd_t fd, void * userdata, child_proc_exited_t on_pid_exit );

bool_t child_proc_mgr_start ( struct _child_proc_mgr * cpmgr, p_io_scheduler_t scheduler );

void child_proc_mgr_stop ( struct _child_proc_mgr * cpmgr );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* CHILD_PROCESS_MGR_H__ */
