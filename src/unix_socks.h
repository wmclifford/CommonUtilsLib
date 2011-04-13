/**
 * @file    unix_socks.h
 * @author  William Clifford
 **/

#ifndef UNIX_SOCKS_H__
#define UNIX_SOCKS_H__

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

sock_fd_t unix_create_bound_dgram_socket ( const char * filename );

sock_fd_t unix_create_bound_stream_socket ( const char * filename );

sock_fd_t unix_create_client_dgram_socket ( const char * filename );

sock_fd_t unix_create_client_stream_socket ( const char * filename );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* UNIX_SOCKS_H__ */
