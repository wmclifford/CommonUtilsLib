/**
 * @file udp_socks.c
 * @author William Clifford
 *
 **/

#include "udp_socks.h"

#define CATEGORY_NAME "udp"
#include "logging-svc.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

sock_fd_t
udp_create_bound_socket ( uint16_t udp_port )
{
  return udp_create_bound_socket_full ( INADDR_ANY, udp_port );
}

sock_fd_t
udp_create_bound_socket_full ( in_addr_t ip_address, uint16_t udp_port )
{
  sock_fd_t rv;
  struct sockaddr_in local_addr;
  int flags = 1;
  
  rv = socket ( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
  if ( rv != INVALID_SOCKET_FD ) {
    if ( setsockopt ( rv, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags) ) < 0 ) {
      close ( rv );
      return INVALID_SOCKET_FD;
    }
    memset ( &local_addr, 0, sizeof( struct sockaddr_in ) );
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = ip_address;
    local_addr.sin_port = htons ( udp_port );
    if ( bind ( rv, (struct sockaddr*) &local_addr, sizeof( struct sockaddr_in ) ) < 0 ) {
      close ( rv );
      rv = INVALID_SOCKET_FD;
    }
  }
  
  return rv;
}

sock_fd_t
udp_create_bound_socket_full_s ( const char * ip_address_str, uint16_t udp_port )
{
  in_addr_t ip;
  if ( inet_aton ( ip_address_str, (struct in_addr*) &ip ) )
    return udp_create_bound_socket_full ( ip, udp_port );
  else
    return INVALID_SOCKET_FD;
}

sock_fd_t
udp_create_client_socket ( void )
{
  return socket ( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
}

bool_t
udp_join_multicast_group ( sock_fd_t udp_sock_fd, in_addr_t local_ip, in_addr_t multicast_ip )
{
  struct ip_mreq mcast_request;
  if ( udp_sock_fd == INVALID_SOCKET_FD )
    return CMNUTIL_FALSE;
  mcast_request.imr_multiaddr.s_addr = multicast_ip;
  mcast_request.imr_interface.s_addr = local_ip;
  return ( setsockopt ( udp_sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                        (void*) &mcast_request, sizeof( struct ip_mreq ) ) == 0 );
}

bool_t
udp_join_multicast_group_s ( sock_fd_t udp_sock_fd, const char * local_ip_str, const char * multicast_ip_str )
{
  in_addr_t local_ip;
  in_addr_t multi_ip;
  
  if ( inet_aton ( local_ip_str, (struct in_addr*) &local_ip ) &&
       inet_aton ( multicast_ip_str, (struct in_addr*) &multi_ip ) )
    return udp_join_multicast_group ( udp_sock_fd, local_ip, multi_ip );
  else
    return CMNUTIL_FALSE;
}

bool_t
udp_leave_multicast_group ( sock_fd_t udp_sock_fd, in_addr_t local_ip, in_addr_t multicast_ip )
{
  struct ip_mreq mcast_request;
  if ( udp_sock_fd == INVALID_SOCKET_FD )
    return CMNUTIL_FALSE;
  mcast_request.imr_multiaddr.s_addr = multicast_ip;
  mcast_request.imr_interface.s_addr = local_ip;
  return ( setsockopt ( udp_sock_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                        (void*) &mcast_request, sizeof( struct ip_mreq ) ) == 0 );
}

bool_t
udp_leave_multicast_group_s ( sock_fd_t udp_sock_fd, const char * local_ip_str, const char * multicast_ip_str )
{
  in_addr_t local_ip;
  in_addr_t multi_ip;
  
  if ( inet_aton ( local_ip_str, (struct in_addr*) &local_ip ) &&
       inet_aton ( multicast_ip_str, (struct in_addr*) &multi_ip ) )
    return udp_leave_multicast_group ( udp_sock_fd, local_ip, multi_ip );
  else
    return CMNUTIL_FALSE;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
