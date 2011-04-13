/**
 * @file socket-mgr.c
 * @author William Clifford
 *
 **/

#include "socket-mgr.h"
#define CATEGORY_NAME "socket-mgr"
#include "logging-svc.h"
#include "tcp_socks.h"
#include "udp_socks.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

typedef struct _sockmgr_sockinf {
  struct _sockmgr_sockinf * next;
  uint16_t port;
  sock_fd_t sockfd;
  size_t connections;
} sockmgr_sockinf_t, * p_sockmgr_sockinf_t;

#define SOCKMGR_SOCKINF_STRUCT_SIZE     (sizeof( struct _sockmgr_sockinf ))
#define NIL_SOCKINF                     ((p_sockmgr_sockinf_t) 0)

typedef struct _sockmgr_event {
  struct _sockmgr_event * next;
  sockmgr_socket_closed_cbk_t cbkfn;
} sockmgr_event_t, * p_sockmgr_event_t;

#define SOCKMGR_EVENT_STRUCT_SIZE       (sizeof( struct _sockmgr_event ))
#define NIL_EVENT                       ((p_sockmgr_event_t) 0)

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Shared (global) variables        */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local function prototypes        */
/* ---------- ---------- ---------- */

static inline void close_socket ( p_sockmgr_sockinf_t * listhead, sock_fd_t sockfd );
static inline p_sockmgr_sockinf_t create_sockinf ( uint16_t port, sock_fd_t sockfd );
static inline void notify_listeners_socket_closed ( sock_fd_t sockfd );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Module variables      */
/* ---------- ---------- */

static pthread_mutex_t mutex_tcp_socket_list = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_udp_socket_list = PTHREAD_MUTEX_INITIALIZER;

static p_sockmgr_event_t sockmgr_event_list = NIL_EVENT;
static p_sockmgr_sockinf_t sockmgr_tcp_socket_list = NIL_SOCKINF;
static p_sockmgr_sockinf_t sockmgr_udp_socket_list = NIL_SOCKINF;

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

void
sockmgr_add_socket_closed_evhandler ( sockmgr_socket_closed_cbk_t onclosecbk )
{
  p_sockmgr_event_t
    pnewhandler,
    pevhandler = sockmgr_event_list;
  
  /* Must provide a valid callback function */
  if ( !( onclosecbk ) )
    return;
  
  pnewhandler = (p_sockmgr_event_t) malloc ( SOCKMGR_EVENT_STRUCT_SIZE );
  if ( pnewhandler ) {
    pnewhandler->next = NIL_EVENT;
    pnewhandler->cbkfn = onclosecbk;
    if ( pevhandler == NIL_EVENT )
      sockmgr_event_list = pnewhandler;
    else {
      while ( pevhandler->next != NIL_EVENT )
        pevhandler = pevhandler->next;
      pevhandler->next = pnewhandler;
    }
  }
}

/**
 * Requests that the TCP socket be closed.
 * All sockets are instance counted; once the instance count reaches zero, the socket will
 * actually be closed. Otherwise the instance count is merely decremented by one.
 **/
void
sockmgr_close_tcp ( sock_fd_t sockfd )
{
  LOCK_MUTEX( mutex_tcp_socket_list );
  close_socket ( &sockmgr_tcp_socket_list, sockfd );
  UNLOCK_MUTEX( mutex_tcp_socket_list );
}

/**
 * Requests that the UDP socket be closed.
 * All sockets are instance counted; once the instance count reaches zero, the socket will
 * actually be closed. Otherwise the instance count is merely decremented by one.
 **/
void
sockmgr_close_udp ( sock_fd_t sockfd )
{
  LOCK_MUTEX( mutex_udp_socket_list );
  close_socket ( &sockmgr_udp_socket_list, sockfd );
  UNLOCK_MUTEX( mutex_udp_socket_list );
}

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
sock_fd_t
sockmgr_get_or_create_tcp ( uint16_t port )
{
  sock_fd_t rv = INVALID_SOCKET_FD;
  p_sockmgr_sockinf_t pinf = NIL_SOCKINF;
  
  LOCK_MUTEX( mutex_tcp_socket_list );
  
  if ( sockmgr_tcp_socket_list ) {
    if ( sockmgr_tcp_socket_list->port == port ) {
      sockmgr_tcp_socket_list->connections++;
      rv = sockmgr_tcp_socket_list->sockfd;
    }
    else {
      pinf = sockmgr_tcp_socket_list;
      while ( pinf->next ) {
        if ( pinf->next->port == port ) {
          pinf->next->connections++;
          rv = pinf->next->sockfd;
          break;
        }
        pinf = pinf->next;
      }
    }
  }
  
  if ( rv == INVALID_SOCKET_FD ) {
    /* Wasn't found */
    rv = tcp_create_bound_socket ( port );
    if ( rv != INVALID_SOCKET_FD ) {
      if ( sockmgr_tcp_socket_list )
        pinf->next = create_sockinf ( port, rv );
      else
        sockmgr_tcp_socket_list = create_sockinf ( port, rv );
    }
  }
  
  UNLOCK_MUTEX( mutex_tcp_socket_list );
  
  return rv;
}

/**
 * Requests a UDP socket bound to the requested port.
 * All sockets are instance counted. If there is already a socket for the given port, the
 * instance count will simply be incremented and the current socket will be returned. If
 * no socket already exists for the port, one is created and stored for future requests,
 * its instance count set to one.
 * @return The UDP socket bound to the requested port, or INVALID_SOCKET_FD on error.
 **/
sock_fd_t
sockmgr_get_or_create_udp ( uint16_t port )
{
  sock_fd_t rv = INVALID_SOCKET_FD;
  p_sockmgr_sockinf_t pinf = NIL_SOCKINF;
  
  LOCK_MUTEX( mutex_udp_socket_list );
  
  if ( sockmgr_udp_socket_list ) {
    if ( sockmgr_udp_socket_list->port == port ) {
      sockmgr_udp_socket_list->connections++;
      rv = sockmgr_udp_socket_list->sockfd;
    }
    else {
      pinf = sockmgr_udp_socket_list;
      while ( pinf->next ) {
        if ( pinf->next->port == port ) {
          pinf->next->connections++;
          rv = pinf->next->sockfd;
          break;
        }
        pinf = pinf->next;
      }
    }
  }
  
  if ( rv == INVALID_SOCKET_FD ) {
    /* Wasn't found */
    rv = udp_create_bound_socket ( port );
    if ( rv != INVALID_SOCKET_FD ) {
      if ( sockmgr_udp_socket_list )
        pinf->next = create_sockinf ( port, rv );
      else
        sockmgr_udp_socket_list = create_sockinf ( port, rv );
    }
  }
  
  UNLOCK_MUTEX( mutex_udp_socket_list );
  
  return rv;
}

/**
 * Closes all sockets. This should only be called before terminating the application, or
 * when a complete restart is taking place.
 **/
void
sockmgr_shutdown ( void )
{
  p_sockmgr_sockinf_t ptmp;
  
  /* Shut down TCP sockets. */
  LOGSVC_DEBUG( "Shutting down TCP sockets" );
  while ( sockmgr_tcp_socket_list != NIL_SOCKINF ) {
    LOGSVC_TRACE( "Closing port: %5d", sockmgr_tcp_socket_list->port );
    close ( sockmgr_tcp_socket_list->sockfd );
    ptmp = sockmgr_tcp_socket_list->next;
    free ( sockmgr_tcp_socket_list );
    sockmgr_tcp_socket_list = ptmp;
  }
  
  /* Shut down UDP sockets. */
  LOGSVC_DEBUG( "Shutting down UDP sockets" );
  while ( sockmgr_udp_socket_list != NIL_SOCKINF ) {
    LOGSVC_TRACE( "Closing port: %5d", sockmgr_udp_socket_list->port );
    close ( sockmgr_udp_socket_list->sockfd );
    ptmp = sockmgr_udp_socket_list->next;
    free ( sockmgr_udp_socket_list );
    sockmgr_udp_socket_list = ptmp;
  }
  
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

static inline void
close_socket ( p_sockmgr_sockinf_t * listhead, sock_fd_t sockfd )
{
  p_sockmgr_sockinf_t pinf, pnext, phead;
  
  if ( !(listhead) || !(*listhead) ) /* make sure a list was passed to us */
    return;
  
  phead = *listhead;
    /* Check the head item first. */
  if ( phead->sockfd == sockfd ) {
    phead->connections--;
    if ( phead->connections < 1 ) {
      close ( sockfd );
      notify_listeners_socket_closed ( sockfd );
      pinf = phead->next;
      free ( phead );
      *listhead = pinf;
    }
  }
  else {
      /* Check the others. */
    pinf = phead;
    while ( pinf->next ) {
      if ( pinf->next->sockfd != sockfd ) {
        pinf = pinf->next;
        continue;
      }
      pinf->next->connections--;
      if ( pinf->next->connections > 0 )
        return;
      close ( sockfd );
      notify_listeners_socket_closed ( sockfd );
      pnext = pinf->next->next;
      free ( pinf->next );
      pinf->next = pnext;
      return;
    }
  }
}

static inline p_sockmgr_sockinf_t
create_sockinf ( uint16_t port, sock_fd_t sockfd )
{
  p_sockmgr_sockinf_t pnewinf = (p_sockmgr_sockinf_t) malloc ( SOCKMGR_SOCKINF_STRUCT_SIZE );
  if ( pnewinf ) {
    memset ( pnewinf, 0, SOCKMGR_SOCKINF_STRUCT_SIZE );
    pnewinf->port = port;
    pnewinf->sockfd = sockfd;
    pnewinf->connections = 1;
  }
  return pnewinf;
}

static inline void
notify_listeners_socket_closed ( sock_fd_t sockfd )
{
  p_sockmgr_event_t pevhnd = sockmgr_event_list;
  
  while ( pevhnd ) {
    pevhnd->cbkfn ( sockfd );
    pevhnd = pevhnd->next;
  }
  
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
