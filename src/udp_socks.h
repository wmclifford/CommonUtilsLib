/**
 * @file    udp_socks.h
 * @brief   Common socket operations for UDP-style sockets.
 * @author  William Clifford
 * @todo    Finish documentation
 **/

#ifndef UDP_SOCKS_H__
#define UDP_SOCKS_H__

#include "gccpch.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

/**
 * @brief Create a UDP style socket binding it to a given port.
 *
 * Attempts to create a UDP socket that is bound to a particular port number.  This
 * essentially creates a "server" socket to which remote hosts may send UDP traffic. The
 * bound address is not bound to any particular interface or IP address; only the port
 * number is bound.
 *
 * @param udp_port The port number to which the new UDP socket should be bound.
 * @return The file descriptor of the new UDP socket on success; otherwise,
 * INVALID_SOCKET_FD is returned.
 **/
sock_fd_t udp_create_bound_socket ( uint16_t udp_port );

/**
 * @brief Create a UDP style socket binding it to a given IP address and port.
 *
 * Attempts to create a UDP socket that is bound to a particular IP address and port
 * number. Like udp_create_bound_socket, this creates a "server" socket to which remote
 * hosts may send UDP traffic. However, this function also binds the socket to a particular
 * IP address / network interface.
 *
 * @param ip_address  The IP address to which the socket is bound, in network byte order.
 * @param udp_port    The port number to which the new UDP socket should be bound.
 * @return The file descriptor of the new UDP socket on success; otherwise,
 * INVALID_SOCKET_FD is returned.
 **/
sock_fd_t udp_create_bound_socket_full ( in_addr_t ip_address, uint16_t udp_port );

/**
 * @brief Create a UDP style socket binding it to a given IP address and port.
 *
 * Attempts to create a UDP socket that is bound to a particular IP address and port
 * number. Like udp_create_bound_socket, this creates a "server" socket to which remote
 * hosts may send UDP traffic. However, this function also binds the socket to a particular
 * IP address / network interface.
 *
 * @param ip_address_str  The IP address to which the socket is bound, as a string.
 * @param udp_port        The port number to which the new UDP socket should be bound.
 * @return The file descriptor of the new UDP socket on success; otherwise,
 * INVALID_SOCKET_FD is returned.
 **/
sock_fd_t udp_create_bound_socket_full_s ( const char * ip_address_str, uint16_t udp_port );

/**
 * @brief Creates a UDP style socket, generally used as a client.
 *
 * Unlike udp_create_bound_socket, this only attempts to open a UDP socket.  The address and
 * port are not taken into consideration and are left unknown.
 *
 * @return The file descriptor of the new UDP socket on success; otherwise,
 * INVALID_SOCKET_FD is returned.
 **/
sock_fd_t udp_create_client_socket ( void );

bool_t udp_join_multicast_group ( sock_fd_t udp_sock_fd, in_addr_t local_ip, in_addr_t multicast_ip );

bool_t udp_join_multicast_group_s ( sock_fd_t udp_sock_fd, const char * local_ip_str, const char * multicast_ip_str );

bool_t udp_leave_multicast_group ( sock_fd_t udp_sock_fd, in_addr_t local_ip, in_addr_t multicast_ip );

bool_t udp_leave_multicast_group_s ( sock_fd_t udp_sock_fd, const char * local_ip_str, const char * multicast_ip_str );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* UDP_SOCKS_H__ */
