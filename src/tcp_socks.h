/**
 * @file    tcp_socks.h
 * @brief   Common socket operations for TCP-style sockets.
 * @author  William Clifford
 * @todo    Document module.
 **/

#ifndef TCP_SOCKS_H__
#define TCP_SOCKS_H__

/* Include the precompiled header for all the standard library includes and project-wide
   definitions. */
#include "gccpch.h"

#include "io-scheduler.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

typedef void ( *tcp_callback_t )( sock_fd_t sockfd, int errcode );

typedef void ( *tcp_callback_ud_t )( p_io_scheduler_t scheduler, sock_fd_t sockfd, int errcode, void * userdata );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

sock_fd_t tcp_accept ( sock_fd_t sockfd );

sock_fd_t tcp_accept_full ( sock_fd_t sockfd,
			    in_addr_t * remote_ip,
			    uint16_t * remote_port );

sock_fd_t tcp_accept_full_s ( sock_fd_t sockfd,
			      char * remote_ip_str, size_t remote_ip_len,
			      uint16_t * remote_port );

bool_t tcp_connect ( sock_fd_t sockfd,
                     in_addr_t remote_ip, uint16_t remote_port,
                     p_io_scheduler_t scheduler,
                     tcp_callback_t on_conn_cbk );

bool_t tcp_connect_s ( sock_fd_t sockfd,
                       const char * remote_ip_str, uint16_t remote_port,
                       p_io_scheduler_t scheduler,
                       tcp_callback_t on_conn_cbk );

bool_t tcp_connect_timeout ( sock_fd_t sockfd,
                             in_addr_t remote_ip, uint16_t remote_port,
                             p_io_scheduler_t scheduler,
                             tcp_callback_t on_conn_cbk,
                             int timeout_secs );

bool_t tcp_connect_timeout_ud ( sock_fd_t sockfd,
                                in_addr_t remote_ip, uint16_t remote_port,
                                p_io_scheduler_t scheduler, void * userdata,
                                tcp_callback_ud_t on_conn_cbk, int timeout_secs );

sock_fd_t tcp_create_bound_socket ( uint16_t local_port );

sock_fd_t tcp_create_bound_socket_full ( in_addr_t local_ip, uint16_t local_port );

sock_fd_t tcp_create_bound_socket_full_s ( const char * local_ip_str, uint16_t local_port );

sock_fd_t tcp_create_client_socket ( void );

ssize_t tcp_receive ( sock_fd_t sockfd, void * buffer, size_t buffer_size );

ssize_t tcp_send ( sock_fd_t sockfd, const void * data, size_t data_length );

void tcp_set_socket_nonblocking ( sock_fd_t sockfd, bool_t onOff );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* TCP_SOCKS_H__ */
