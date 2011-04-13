/**
 * @file    unix_socks.c
 * @author  William Clifford
 *
 **/

#include "unix_socks.h"

// This is not currently included in the pre-compiled header; these types of sockets are
// not currently used in any large enough capacity to warrant putting it in yet.
//
#ifndef __USE_MISC
#define __USE_MISC
#endif

#include <sys/un.h>

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

sock_fd_t
unix_create_bound_dgram_socket ( const char * filename )
{
  struct stat sb;
  struct sockaddr_un addr;
  int addrlen;
  sock_fd_t rv;
  
  memset ( &addr, 0, sizeof( struct sockaddr_un ) );
  memset ( &sb, 0, sizeof( struct stat ) );
  
  //
  // Make sure file does not already exist. If it does, and it is a socket, kill it off.
  //
  if ( ( stat ( filename, &sb ) == 0 ) && ( S_ISSOCK( sb.st_mode ) ) ) {
    unlink ( filename );
  }
  
  rv = socket ( PF_UNIX, SOCK_DGRAM, 0 );
  if ( rv != INVALID_SOCKET_FD ) {
    addr.sun_family = AF_UNIX;
    strncat ( addr.sun_path, filename, sizeof( addr.sun_path ) );
    addrlen = SUN_LEN( &addr );
    
    if ( bind ( rv, (struct sockaddr*) &addr, addrlen ) == -1 ) {
      close ( rv );
      rv = INVALID_SOCKET_FD;
    }
  }
  
  return rv;
}

sock_fd_t
unix_create_bound_stream_socket ( const char * filename )
{
  struct stat sb;
  struct sockaddr_un addr;
  int addrlen;
  sock_fd_t rv;
  
  memset ( &addr, 0, sizeof( struct sockaddr_un ) );
  memset ( &sb, 0, sizeof( struct stat ) );
  
  //
  // Make sure file does not already exist. If it does, and it is a socket, kill it off.
  //
  if ( ( stat ( filename, &sb ) == 0 ) && ( S_ISSOCK( sb.st_mode ) ) ) {
    unlink ( filename );
  }
  
  rv = socket ( PF_UNIX, SOCK_STREAM, 0 );
  if ( rv != INVALID_SOCKET_FD ) {
    addr.sun_family = AF_UNIX;
    strncat ( addr.sun_path, filename, sizeof( addr.sun_path ) );
    addrlen = SUN_LEN( &addr );
    
    if ( bind ( rv, (struct sockaddr*) &addr, addrlen ) == -1 ) {
      close ( rv );
      rv = INVALID_SOCKET_FD;
    }
  }
  
  return rv;
}

sock_fd_t
unix_create_client_dgram_socket ( const char * filename )
{
  struct sockaddr_un addr;
  int addrlen;
  sock_fd_t rv;
  
  memset ( &addr, 0, sizeof( struct sockaddr_un ) );
  
  rv = socket ( PF_UNIX, SOCK_DGRAM, 0 );
  if ( rv != INVALID_SOCKET_FD ) {
    addr.sun_family = AF_UNIX;
    strncat ( addr.sun_path, filename, sizeof( addr.sun_path ) );
    addrlen = SUN_LEN( &addr );
    
    if ( connect ( rv, (struct sockaddr*) &addr, addrlen ) == -1 ) {
      close ( rv );
      rv = INVALID_SOCKET_FD;
    }
  }
  
  return rv;
}

sock_fd_t
unix_create_client_stream_socket ( const char * filename )
{
  struct sockaddr_un addr;
  int addrlen;
  sock_fd_t rv;
  
  memset ( &addr, 0, sizeof( struct sockaddr_un ) );
  
  rv = socket ( PF_UNIX, SOCK_STREAM, 0 );
  if ( rv != INVALID_SOCKET_FD ) {
    addr.sun_family = AF_UNIX;
    strncat ( addr.sun_path, filename, sizeof( addr.sun_path ) );
    addrlen = SUN_LEN( &addr );
    
    if ( connect ( rv, (struct sockaddr*) &addr, addrlen ) == -1 ) {
      close ( rv );
      rv = INVALID_SOCKET_FD;
    }
  }
  
  return rv;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
