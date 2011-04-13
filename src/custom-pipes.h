/**
 * @file    custom-pipes.h
 * @author  William Clifford
 **/

#ifndef CUSTOM_PIPES_H__
#define CUSTOM_PIPES_H__

/* Include the precompiled header for all the standard library includes and project-wide definitions. */
#include "gccpch.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Constants  */
/* ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Type definitions and structures  */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

pid_t my_popen ( fd_t * pin, fd_t * pout, fd_t * perr, const char * fmt, ... );

char * my_system ( int * result_length, const char * fmt, ... );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* CUSTOM_PIPES_H__ */
