/**
 * @file    process_mgmt.h
 * @author  William Clifford
 * 
 **/

#ifndef PROCESS_MGMT_H__
#define PROCESS_MGMT_H__

/* Include the precompiled header for all the standard library includes and project-wide
   definitions. */
#include "gccpch.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

/**
 * @brief Attempts to send a signal to the given process and determine if it is still alive.
 * @param process_pid The PID of the process we are looking for.
 * @return True if process is still "alive"; otherwise, false.
 **/
bool_t proc_mgmt_is_pid_alive ( pid_t process_pid );

/**
 * @brief Locates the PID file for the named process, and sees if it is still alive.
 * @param process_name The name of the process we are checking.
 * @param p_process_pid Receives the PID of the process.
 * @return True if process is still "alive"; otherwise, false.
 * 
 * Makes use of the "/var/run" folder, looking for a file there named "{process_name}.pid",
 * and, if found, reads the process ID from it. That PID is then "ping"-ed to see if it is
 * still running.
 **/
bool_t proc_mgmt_is_process_alive ( const char * process_name, pid_t * p_process_pid );

/**
 * @brief Records the process ID of the current process to a file in the "/var/run" folder.
 * @param process_name The name of the current process.
 * @return True if able to store the PID to the file; otherwise, false.
 **/
bool_t proc_mgmt_record_my_pid ( const char * process_name );

/**
 * @brief Records a process ID of the named process to a file in the "/var/run" folder.
 * @param process_name The name of the process.
 * @param process_pid The process ID.
 * @return True if able to store the PID to the file; otherwise, false.
 **/
bool_t proc_mgmt_record_pid ( const char * process_name, pid_t process_pid );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* PROCESS_MGMT_H__ */
