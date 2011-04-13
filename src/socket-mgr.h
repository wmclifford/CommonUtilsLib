/**
 * @file    socket-mgr.h
 * @author  William Clifford
 * 
 * This module is designed to manage all the listening sockets that the MFIP device
 * keeps open during the lifetime of the application. Some are TCP sockets, others
 * are UDP. Users of this module request which type they want, and the port number
 * on which they want to open a listening socket. If a socket is already open for that
 * port, it is returned; otherwise, a new socket is created and returned to the
 * caller.
 **/

#ifndef SOCKET_MGR_H__
#define SOCKET_MGR_H__

/* Include the precompiled header for all the standard library includes and project-wide
   definitions. */
#include "gccpch.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

/**
 * Callback function type for notifications when a socket is closed.
 **/
typedef void ( *sockmgr_socket_closed_cbk_t ) ( sock_fd_t sockfd );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

void sockmgr_add_socket_closed_evhandler ( sockmgr_socket_closed_cbk_t onclosecbk );

/**
 * Requests that the TCP socket be closed.
 * All sockets are instance counted; once the instance count reaches zero, the socket will
 * actually be closed. Otherwise the instance count is merely decremented by one.
 **/
void sockmgr_close_tcp ( sock_fd_t sockfd );

/**
 * Requests that the UDP socket be closed.
 * All sockets are instance counted; once the instance count reaches zero, the socket will
 * actually be closed. Otherwise the instance count is merely decremented by one.
 **/
void sockmgr_close_udp ( sock_fd_t sockfd );

/**
 * Requests a TCP socket listening on the requested port.
 * All sockets are instance counted. If there is already a socket for the given port, the
 * instance count will simply be incremented and the current socket will be returned. If
 * no socket already exists for the port, one is created and stored for future requests,
 * its instance count set to one.
 * Note that the TCP socket is bound to all network interfaces via INADDR_ANY; if the
 * socket must be bound to a specific interface, another function must be added here. For
 * now, we assume that the interface does not matter.
 * @return The TCP socket listening on the requested port, or INVALID_SOCKET_FD on error.
 **/
sock_fd_t sockmgr_get_or_create_tcp ( uint16_t port );

/**
 * Requests a UDP socket bound to the requested port.
 * All sockets are instance counted. If there is already a socket for the given port, the
 * instance count will simply be incremented and the current socket will be returned. If
 * no socket already exists for the port, one is created and stored for future requests,
 * its instance count set to one.
 * @return The UDP socket bound to the requested port, or INVALID_SOCKET_FD on error.
 **/
sock_fd_t sockmgr_get_or_create_udp ( uint16_t port );

/**
 * Closes all sockets. This should only be called before terminating the application, or
 * when a complete restart is taking place.
 **/
void sockmgr_shutdown ( void );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* _H__ */
