
#include "tcp_socks.h"

#define CATEGORY_NAME "tcp"
#include "logging-svc.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

/* Since the TCP connection may not complete immediately, and we do not want to block
   our application with waiting TCP sockets, we have to keep track of any pending TCP
   client connections that are being opened. */

struct _pending_connection
{
  struct _pending_connection *  next;
  sock_fd_t                     sockfd;
  int                           status;
  tcp_callback_t                on_connect;
  tcp_callback_ud_t             on_connect_ud;
  void *                        user_data;
  struct sockaddr_in            remote_addr;
};

typedef struct _pending_connection * p_pending_connection;

#define PENDING_CONNECTION_STRUCT_SIZE           (sizeof( struct _pending_connection ))

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Shared (global) variables        */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local function prototypes        */
/* ---------- ---------- ---------- */

static bool_t tcp_io_scheduler_connect_cbk ( p_io_scheduler_task_t task, int errcode );
static void tcp_pending_connection_free ( p_pending_connection pconn );
static p_pending_connection tcp_pending_connection_new ( sock_fd_t sockfd, in_addr_t ip_addr, uint16_t port );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Module variables      */
/* ---------- ---------- */

static p_pending_connection pending_connections = (p_pending_connection) 0;

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

sock_fd_t
tcp_accept ( sock_fd_t sockfd )
{
  return tcp_accept_full ( sockfd, NULL, NULL );
}

sock_fd_t
tcp_accept_full ( sock_fd_t sockfd, in_addr_t * remote_ip, uint16_t * remote_port )
{
  struct sockaddr_in remote_addr;
  socklen_t remote_addr_len = sizeof ( struct sockaddr_in );
  int flags;
  sock_fd_t remote_sock_fd;

  /* As per the man page for accept(), make sure that the socket is set to non-blocking before
     calling accept(). */
  flags = fcntl ( sockfd, F_GETFL );
  fcntl ( sockfd, F_SETFL, flags | O_NONBLOCK );
  remote_sock_fd = accept ( sockfd, (struct sockaddr*) &remote_addr, &remote_addr_len );
  fcntl ( sockfd, F_SETFL, flags );

  if ( remote_sock_fd != INVALID_SOCKET_FD ) {
    flags = 0;
    setsockopt ( remote_sock_fd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof ( flags ) );
    if ( remote_ip )
      *remote_ip = remote_addr.sin_addr.s_addr; /* Remember that this is in network byte order. */
    if ( remote_port )
      *remote_port = ntohs ( remote_addr.sin_port );
  }

  return remote_sock_fd;
}

sock_fd_t
tcp_accept_full_s ( sock_fd_t sockfd, char * remote_ip_str, size_t remote_ip_len, uint16_t * remote_port )
{
  struct in_addr remote_ip;
  sock_fd_t rv = tcp_accept_full ( sockfd, (in_addr_t*) &remote_ip, remote_port );
  if ( ( rv != INVALID_SOCKET_FD ) && ( remote_ip_str ) ) {
    *remote_ip_str = 0;
    strncat ( remote_ip_str, inet_ntoa ( remote_ip ), remote_ip_len );
  }
  return rv;
}

bool_t
tcp_connect ( sock_fd_t sockfd, in_addr_t remote_ip, uint16_t remote_port,
              p_io_scheduler_t scheduler, tcp_callback_t on_conn_cbk )
{
  return tcp_connect_timeout ( sockfd, remote_ip, remote_port, scheduler, on_conn_cbk, 2 );
}

bool_t
tcp_connect_s ( sock_fd_t sockfd, const char * remote_ip_str, uint16_t remote_port,
                p_io_scheduler_t scheduler, tcp_callback_t on_conn_cbk )
{
  in_addr_t ip;
  if ( inet_aton ( remote_ip_str, (struct in_addr*) &ip ) )
    return tcp_connect_timeout ( sockfd, ip, remote_port, scheduler, on_conn_cbk, 2 );
  else
    return ICP_FALSE;
}

bool_t
tcp_connect_timeout ( sock_fd_t sockfd, in_addr_t remote_ip, uint16_t remote_port,
                      p_io_scheduler_t scheduler, tcp_callback_t on_conn_cbk, int timeout_secs )
{
  p_io_scheduler_task_t task;
  int rc;

  if ( sockfd == INVALID_SOCKET_FD ) {
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): received bad file descriptor" );
    return ICP_FALSE;
  }

  p_pending_connection pconn = tcp_pending_connection_new ( sockfd, remote_ip, remote_port );
  if ( !pconn ) {
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): failed to create a new pending connection" );
    return ICP_FALSE;
  }

  /* Set socket to non-blocking mode. */
  LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): setting socket to non-blocking mode" );
  if( fcntl ( sockfd, F_SETFL, fcntl ( sockfd, F_GETFL ) | O_NONBLOCK ) == -1 )
    LOGSVC_ERROR( CATEGORY_NAME, "tcp_connect_timeout(): unable to set to non-blocking mode" );

  /* Attempt connection; it may connect immediately, most likely not. We need to check if
     the return code of the connect() call is -1, and if so, if errno is set to
     EINPROGRESS. Any other error code means the connection failed. Otherwise we add our
     connection to the list of pending connections and add an item to our IO scheduler. */
  LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): calling connect()" );
  rc = connect ( sockfd, (struct sockaddr*) &(pconn->remote_addr), sizeof ( struct sockaddr_in ) );
  if ( rc == 0 ) {
    /* Connected immediately; we can go ahead and call the callback function we were
       passed. */
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): connect() returned 0" );
    tcp_pending_connection_free ( pconn );
    if ( on_conn_cbk ) {
      LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): calling on_conn_cbk" );
      on_conn_cbk ( sockfd, 0 );
    }
    return ICP_TRUE;
  }
  else if ( ( rc == -1 ) && ( errno != EINPROGRESS ) ) {
    /* Connection failed immediately. */
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): connect() failed immediately" );
    tcp_pending_connection_free ( pconn );
    return ICP_FALSE;
  }
  
  if ( scheduler == NIL_IO_SCHEDULER ) {
    /* No scheduler was given to the function, and the connect did not complete immediately.
       We cannot schedule to watch for when the socket actually connects. */
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): did not receive a valid I/O scheduler" );
    tcp_pending_connection_free ( pconn );
    close ( sockfd );
    return ICP_FALSE;
  }

  /* If we got here, connect() returned -1, and the connection is being attempted (EINPROGRESS). */
  LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): adding a task to watch for connection completion" );
  pconn->on_connect = on_conn_cbk;
  pconn->next = pending_connections;
  pending_connections = pconn;
  task = io_sched_create_writer_task ( scheduler, sockfd, (int64_t) IO_SCHEDULER_TIME_ONE_SECOND * (int64_t) timeout_secs,
                                       (void*) pconn, tcp_io_scheduler_connect_cbk );
  io_sched_schedule_task ( task );
  return ICP_TRUE;
}

bool_t
tcp_connect_timeout_ud ( sock_fd_t sockfd, in_addr_t remote_ip, uint16_t remote_port,
                         p_io_scheduler_t scheduler, void * userdata, tcp_callback_ud_t on_conn_cbk, int timeout_secs )
{
  p_io_scheduler_task_t task;
  int rc;
  
  if ( sockfd == INVALID_SOCKET_FD ) {
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): received bad file descriptor" );
    return ICP_FALSE;
  }
  
  p_pending_connection pconn = tcp_pending_connection_new ( sockfd, remote_ip, remote_port );
  if ( !pconn ) {
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): failed to create a new pending connection" );
    return ICP_FALSE;
  }
  pconn->user_data = userdata;
  
  /* Set socket to non-blocking mode. */
  LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): setting socket to non-blocking mode" );
  if( fcntl ( sockfd, F_SETFL, fcntl ( sockfd, F_GETFL ) | O_NONBLOCK ) == -1 )
    LOGSVC_ERROR( CATEGORY_NAME, "tcp_connect_timeout(): unable to set to non-blocking mode" );
  
  /* Attempt connection; it may connect immediately, most likely not. We need to check if
     the return code of the connect() call is -1, and if so, if errno is set to
     EINPROGRESS. Any other error code means the connection failed. Otherwise we add our
     connection to the list of pending connections and add an item to our IO scheduler. */
  LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): calling connect()" );
  rc = connect ( sockfd, (struct sockaddr*) &(pconn->remote_addr), sizeof ( struct sockaddr_in ) );
  if ( rc == 0 ) {
    /* Connected immediately; we can go ahead and call the callback function we were
       passed. */
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): connect() returned 0" );
    tcp_pending_connection_free ( pconn );
    if ( on_conn_cbk ) {
      LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): calling on_conn_cbk" );
      on_conn_cbk ( scheduler, sockfd, 0, userdata );
    }
    return ICP_TRUE;
  }
  else if ( ( rc == -1 ) && ( errno != EINPROGRESS ) ) {
    /* Connection failed immediately. */
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): connect() failed immediately" );
    tcp_pending_connection_free ( pconn );
    return ICP_FALSE;
  }
  
  if ( scheduler == NIL_IO_SCHEDULER ) {
    /* No scheduler was given to the function, and the connect did not complete immediately.
       We cannot schedule to watch for when the socket actually connects. */
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): did not receive a valid I/O scheduler" );
    tcp_pending_connection_free ( pconn );
    close ( sockfd );
    return ICP_FALSE;
  }
  
  /* If we got here, connect() returned -1, and the connection is being attempted (EINPROGRESS). */
  LOGSVC_TRACE( CATEGORY_NAME, "tcp_connect_timeout(): adding a task to watch for connection completion" );
  pconn->on_connect_ud = on_conn_cbk;
  pconn->next = pending_connections;
  pending_connections = pconn;
  task = io_sched_create_writer_task ( scheduler, sockfd, (int64_t) IO_SCHEDULER_TIME_ONE_SECOND * (int64_t) timeout_secs,
                                       (void*) pconn, tcp_io_scheduler_connect_cbk );
  io_sched_schedule_task ( task );
  return ICP_TRUE;
}

sock_fd_t
tcp_create_bound_socket ( uint16_t local_port )
{
  return tcp_create_bound_socket_full ( INADDR_ANY, local_port );
}

sock_fd_t
tcp_create_bound_socket_full ( in_addr_t local_ip, uint16_t local_port )
{
  sock_fd_t rv;
  struct sockaddr_in local_addr;
  int flags = 0;
  
  rv = socket ( PF_INET, SOCK_STREAM, IPPROTO_TCP );
  if ( rv != INVALID_SOCKET_FD )
  {
    setsockopt ( rv, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof ( flags ) );
    flags = 1;
    setsockopt ( rv, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof ( flags ) );
    memset ( &local_addr, 0, sizeof ( struct sockaddr_in ) );
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = local_ip;
    local_addr.sin_port = htons ( local_port );
    if ( bind ( rv, (struct sockaddr*) &local_addr, sizeof ( struct sockaddr_in ) ) == -1 ) {
      close ( rv );
      rv = INVALID_SOCKET_FD;
    }
    if ( listen ( rv, 5 ) == -1 ) {
      close ( rv );
      rv = INVALID_SOCKET_FD;
    }
  }
  
  return rv;
}

sock_fd_t
tcp_create_bound_socket_full_s ( const char * local_ip_str, uint16_t local_port )
{
  in_addr_t ip;
  if ( inet_aton ( local_ip_str, (struct in_addr*) &ip ) )
    return tcp_create_bound_socket_full ( ip, local_port );
  else
    return INVALID_SOCKET_FD;
}

sock_fd_t
tcp_create_client_socket ( void )
{
  sock_fd_t rv;
  int flags = 0;

  rv = socket ( PF_INET, SOCK_STREAM, IPPROTO_TCP );
  if ( rv != INVALID_SOCKET_FD )
    setsockopt ( rv, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof ( flags ) );
  return rv;
}

ssize_t
tcp_receive ( sock_fd_t sockfd, void * buffer, size_t buffer_size )
{
  if ( ( sockfd == INVALID_SOCKET_FD ) || !( buffer ) )
    return -1;
  else
    return read ( sockfd, buffer, buffer_size );
}

ssize_t
tcp_send ( sock_fd_t sockfd, const void * data, size_t data_length )
{
  ssize_t bytes_sent = 0, tot_bytes_sent = 0;
  size_t bytes_remaining = data_length;
  uint8_t * bytes = (uint8_t*) data;
  if ( ( sockfd != INVALID_SOCKET_FD ) && ( data ) && ( data_length ) ) {
    while ( bytes_remaining > 0 ) {
      bytes_sent = send ( sockfd, bytes, bytes_remaining, MSG_NOSIGNAL );
      if ( bytes_sent == -1 ) {
	if ( ( bytes_sent == EAGAIN ) || ( bytes_sent == EINTR ) )
	  continue;
	return -1;
      }
      else if ( bytes_sent == 0 )
	return 0;
      bytes += bytes_sent;
      bytes_remaining -= bytes_sent;
      tot_bytes_sent += bytes_sent;
    }
  }
  return tot_bytes_sent;
}

void
tcp_set_socket_nonblocking ( sock_fd_t sockfd, bool_t onOff )
{
  int flags;
  if ( sockfd != INVALID_SOCKET_FD ) {
    flags = fcntl ( sockfd, F_GETFL );
    if ( onOff )
      flags |= O_NONBLOCK;
    else
      flags &= ~O_NONBLOCK;
    fcntl ( sockfd, F_SETFL, flags );
  }
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

static bool_t
tcp_io_scheduler_connect_cbk ( p_io_scheduler_task_t task, int errcode )
{
  p_pending_connection pconn;
  int flags;
  sock_fd_t sockfd;
  
  char buf[512];
  int sockerr, rc;
  socklen_t sockerr_len;

  /* We remove the pending connection from our list. */
  pconn = pending_connections;
  if ( pconn == (p_pending_connection) task->user_data )
    pending_connections = pending_connections->next;
  else while ( pconn )
	 {
	   if ( pconn->next == (p_pending_connection) task->user_data )
	     {
	       pconn->next = pconn->next->next;
	       pconn = (p_pending_connection) task->user_data;
	       break;
	     }
	   else
	     pconn = pconn->next;
	 }
  /* If pconn wound up as NULL, then our pending connection was not in the list. Log
     this. */
  if ( !pconn )
    {
      LOGSVC_WARNING( CATEGORY_NAME, "Received IO scheduler callback for untracked pending TCP connection." );
      pconn = (p_pending_connection) task->user_data;
    }
  
  sockfd = task->fd;

  /* See if we timed out while waiting to connect to the server socket. */
  if ( errcode == IO_SCHEDULER_ERR_OP_TIMEOUT )
    {
      LOGSVC_TRACE( CATEGORY_NAME, "tcp_io_scheduler_connect_cbk(): timed out" );
      if ( pconn->on_connect_ud )
        pconn->on_connect_ud ( task->owner, sockfd, 1, pconn->user_data );
      else
        pconn->on_connect ( sockfd, 1 ); /* TODO: Change this "magic number" */
      close ( sockfd );
      return IO_SCHEDULER_TASK_COMPLETE;
    }

  /* Check the SO_ERROR socket "option" to see if the connection was completed successfully or not. */
  sockerr_len = sizeof( sockerr );
  rc = getsockopt ( sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &sockerr_len );
  if ( rc || sockerr ) {
    strerror_r ( (rc ? errno : sockerr), buf, sizeof( buf ) );
    LOGSVC_TRACE( CATEGORY_NAME, "tcp_io_scheduler_connect_cbk(): connection errored - %s", buf );
    if ( pconn->on_connect_ud )
      pconn->on_connect_ud ( task->owner, sockfd, (rc ? errno : sockerr), pconn->user_data );
    else
      pconn->on_connect ( sockfd, (rc ? errno : sockerr) );
    close ( sockfd );
    return IO_SCHEDULER_TASK_COMPLETE;
  }

  /* Connection was successfully established. Set the socket to blocking mode and call the
     on_connect callback. If the socket needs to be in non-blocking mode, it will need to be
     set afterwards. */
  LOGSVC_TRACE( CATEGORY_NAME, "tcp_io_scheduler_connect_cbk(): connection successful" );
  flags = fcntl ( sockfd, F_GETFL ) & ~O_NONBLOCK;
  fcntl ( sockfd, F_SETFL, flags );
  if ( pconn->on_connect_ud )
    pconn->on_connect_ud ( task->owner, sockfd, 0, pconn->user_data );
  else
    pconn->on_connect ( sockfd, 0 ); /* TODO: Change this "magic number" */
  return IO_SCHEDULER_TASK_COMPLETE;
}

static void
tcp_pending_connection_free ( p_pending_connection pconn )
{
  free ( (void*) pconn );
}

static p_pending_connection
tcp_pending_connection_new ( sock_fd_t sockfd, in_addr_t ip_addr, uint16_t port )
{
  p_pending_connection rv = (p_pending_connection) malloc ( PENDING_CONNECTION_STRUCT_SIZE );
  if ( rv )
    {
      memset ( rv, 0, PENDING_CONNECTION_STRUCT_SIZE );
      rv->sockfd = sockfd;
      rv->status = EINPROGRESS;
      rv->remote_addr.sin_family = AF_INET;
      rv->remote_addr.sin_addr.s_addr = ip_addr;
      rv->remote_addr.sin_port = htons ( port );
    }
  return rv;
}
