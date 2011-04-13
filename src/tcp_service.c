/**
 * @file    tcp_service.c
 * @author  William Clifford
 **/

#include "tcp_service.h"

#include "socket-mgr.h"
#include "tcp_socks.h"

// For log messages. Specific services will have their own category name.
#define CATEGORY_NAME "tcp_service"
#include "logging-svc.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Shared (global) variables        */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local function prototypes        */
/* ---------- ---------- ---------- */

static void on_tcp_client_connected_to_server ( p_io_scheduler_t scheduler, sock_fd_t fd, int err, void * userdata );

static bool_t on_tcp_client_server_responded ( p_io_scheduler_task_t task, int errcode );

static bool_t on_tcp_listener_client_request ( p_io_scheduler_task_t task, int errcode );

// I/O scheduler read callback: when listener socket becomes "read" ready, a client is waiting.
static bool_t on_tcp_listener_client_waiting ( p_io_scheduler_task_t task, int errcode );

static void tcp_listener_drop_client ( p_tcp_listener_t listener, p_tcp_remote_client_t remcli );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Module variables      */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

//////////////////////////////////////////////////////////////////////////////////////////
//// tcp_client
//////////////////////////////////////////////////////////////////////////////////////////

bool_t
tcp_client_connect ( p_tcp_client_t client, p_io_scheduler_t scheduler )
{
  assert ( client );
  assert ( scheduler );
  
  if ( client->fd == INVALID_SOCKET_FD )
    client->fd = tcp_create_client_socket ();
  
  if ( client->fd != INVALID_SOCKET_FD ) {
    if ( tcp_connect_timeout_ud ( client->fd, client->remote_ip, client->remote_port,
                                  scheduler, client, on_tcp_client_connected_to_server, 3 ) )
    {
      LOGSVC_DEBUG( "Connecting to '%s:%d' ...", client->remote_ip_str, client->remote_port );
      return CMNUTIL_TRUE;
    }
    else {
      LOGSVC_ERROR( "Failed to connect to '%s:%d'", client->remote_ip_str, client->remote_port );
      close ( client->fd );
      client->fd = INVALID_SOCKET_FD;
      
      // TODO: Do we want to start up a reconnect cycle here?
    }
  }
  
  return CMNUTIL_FALSE;
}

void
tcp_client_destroy ( p_tcp_client_t client )
{
  if ( client ) {
    tcp_client_disconnect ( client );
    free ( client->read_buffer );
    free ( client );
  }
}

void
tcp_client_disconnect ( p_tcp_client_t client )
{
  // We are disconnecting from the server, rather than handling the server closing its side of the socket.
  if ( client ) {
    if ( client->io_task ) {
      // Remember to unschedule the I/O task since we are closing the socket.
      io_sched_unschedule_task ( client->io_task );
      client->io_task = NIL_IO_SCHEDULER_TASK;
    }
    if ( client->fd != INVALID_SOCKET_FD ) {
      LOGSVC_DEBUG( "Closing socket connected to %s:%d", client->remote_ip_str, client->remote_port );
      close ( client->fd );
      client->fd = INVALID_SOCKET_FD;
      if ( client->on_closed )
        client->on_closed ( client, TCP_CLIENT_CLOSED_LOCAL );
    }
  }
}

p_tcp_client_t
tcp_client_init ( const char * rem_ip_str, uint16_t rem_port, size_t buffer_size, void * client_userdata )
{
  p_tcp_client_t rv = NEW_tcp_client();
  if ( rv ) {
    memset ( rv, 0, SIZE_tcp_client );
    rv->fd = INVALID_SOCKET_FD;
    strncat ( rv->remote_ip_str, rem_ip_str, sizeof( rv->remote_ip_str ) );
    rv->remote_port = rem_port;
    inet_aton ( rem_ip_str, (struct in_addr*)(void*) &( rv->remote_ip ) ); // check return value??
    
    rv->read_buffer_size = ( buffer_size > 0 ) ? buffer_size : 512; // default buffer size??
    rv->read_buffer = (char*) malloc ( rv->read_buffer_size );
    if ( !( rv->read_buffer ) ) {
      LOGSVC_ERROR( "tcp_client_init(): Unable to allocate read buffer." );
      free ( rv );
      return NIL_tcp_client;
    }
    
    rv->user_data = client_userdata;
    
    // Callbacks need to be set up by caller of tcp_client_init().
  }
  return rv;
}

bool_t
tcp_client_start ( p_tcp_client_t client, p_io_scheduler_t scheduler )
{
  assert ( client );
  assert ( scheduler );
  assert ( client->io_task == NIL_IO_SCHEDULER_TASK );
  
  LOGSVC_DEBUG( "tcp_client_start(): Starting I/O handler for '%s:%d' ...", client->remote_ip_str, client->remote_port );
  client->io_task =
    io_sched_create_reader_task ( scheduler,
                                  client->fd, IO_SCHEDULER_NO_TIMEOUT, (void*) client,
                                  on_tcp_client_server_responded );
  
  return io_sched_schedule_task ( client->io_task );
}

void
tcp_client_stop ( p_tcp_client_t client )
{
  if ( client ) {
    if ( client->io_task ) {
      LOGSVC_DEBUG( "tcp_client_stop(): Stopping I/O handler for '%s:%d' ...", client->remote_ip_str, client->remote_port );
      io_sched_unschedule_task ( client->io_task );
      client->io_task = NIL_IO_SCHEDULER_TASK;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
//// tcp_listener
//////////////////////////////////////////////////////////////////////////////////////////

void
tcp_listener_destroy ( p_tcp_listener_t listener )
{
  if ( listener ) {
    if ( listener->io_task != NIL_IO_SCHEDULER_TASK ) {
      // Looks like it is still running; need to stop it before we destroy it.
      LOGSVC_DEBUG( "tcp_listener_destroy(): Listener appears to still be running; stopping it." );
      tcp_listener_stop ( listener );
    }
    
    // Should be no clients connected ...
    assert ( listener->clients->next == listener->clients );
    
    // Close up the listening socket, get rid of our clients list & mutex, and free up our instance.
    if ( listener->fd != INVALID_SOCKET_FD ) {
      sockmgr_close_tcp ( listener->fd );
      if ( listener->on_closed )
        listener->on_closed ( listener );
    }
    tcp_remote_client_destroy ( listener->clients );
    pthread_mutex_destroy ( &( listener->clients_list_mutex ) );
    free ( listener );
  }
}

p_tcp_listener_t
tcp_listener_init ( uint16_t port, void * listener_userdata )
{
  p_tcp_listener_t rv = NEW_tcp_listener();
  if ( rv ) {
    memset ( rv, 0, SIZE_tcp_listener );
    rv->port = port;
    rv->fd = sockmgr_get_or_create_tcp ( port );
    if ( rv->fd == INVALID_SOCKET_FD ) {
      // Something went wrong; clean up and return NULL.
      LOGSVC_NOTICE( "tcp_listener_init(): Failed to open TCP socket on port %d.", port );
      free ( rv );
      return NIL_tcp_listener;
    }
    rv->clients = tcp_remote_client_init ( rv, INVALID_SOCKET_FD, INADDR_ANY, 0 );
    assert ( rv->clients );
    rv->clients->next = rv->clients->prev = rv->clients; // circular list; empty when (HEAD->n == HEAD->p == HEAD)
    pthread_mutex_init ( &( rv->clients_list_mutex ), (const pthread_mutexattr_t*) 0 );
    rv->user_data = listener_userdata;
  }
  return rv;
}

bool_t
tcp_listener_start ( p_tcp_listener_t listener, p_io_scheduler_t scheduler )
{
  if ( !( listener ) || !( scheduler ) ) {
    LOGSVC_DEBUG( "tcp_listener_start(): Missing listener or I/O scheduler." );
    return CMNUTIL_FALSE;
  }
  
  // We want to create a reader task to watch the listener socket for incoming connections. These will show up
  // as the socket being "read ready" when checked by select().
  //
  listener->io_task =
    io_sched_create_reader_task ( scheduler,
                                  listener->fd, IO_SCHEDULER_NO_TIMEOUT, (void*) listener,
                                  on_tcp_listener_client_waiting );
  if ( !( io_sched_schedule_task ( listener->io_task ) ) ) {
    LOGSVC_ERROR( "Unable to create/schedule I/O task for listener on port %d", listener->port );
    if ( listener->io_task ) {
      free ( listener->io_task );
      listener->io_task = NIL_IO_SCHEDULER_TASK;
    }
    return CMNUTIL_FALSE;
  }
  
  LOGSVC_INFO( "Listener started for TCP port %d", listener->port );
  return CMNUTIL_TRUE;
}

void
tcp_listener_stop ( p_tcp_listener_t listener )
{
  if ( listener ) {
    // Unschedule our I/O task so that no new clients are added.
    //
    if ( listener->io_task != NIL_IO_SCHEDULER_TASK ) {
      io_sched_unschedule_task ( listener->io_task );
      listener->io_task = NIL_IO_SCHEDULER_TASK;
    }
    
    // Since we are stopping the listener, we stop all the clients as well.
    // TODO: look into this; do we really want to stop all the clients at this point? Would it be better to separate this step out?
    //
    p_tcp_remote_client_t remcli, nn;
    LOCK_MUTEX( listener->clients_list_mutex );
    while ( listener->clients->next != listener->clients ) {
      // Unlink the client from the list; no need to worry about the back links here since we are clearing the list.
      nn = listener->clients->next;
      listener->clients->next = nn->next;
      tcp_remote_client_stop ( nn );
      tcp_remote_client_destroy ( nn );
    }
    // At this point, the list is empty, and HEAD->N == HEAD; remember to reset HEAD->P to HEAD.
    listener->clients->prev = listener->clients;
    UNLOCK_MUTEX( listener->clients_list_mutex );
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
//// tcp_remote_client
//////////////////////////////////////////////////////////////////////////////////////////

void
tcp_remote_client_destroy ( p_tcp_remote_client_t remcli )
{
  if ( remcli ) {
    // The task does not get free()-ed here; only needs to be unscheduled -
    // the scheduler will take care of releasing the memory.
    if ( remcli->io_task )
      io_sched_unschedule_task ( remcli->io_task );
    if ( remcli->fd != INVALID_SOCKET_FD )
      close ( remcli->fd );
    free ( remcli );
  }
}

p_tcp_remote_client_t
tcp_remote_client_init ( p_tcp_listener_t owner,
                         sock_fd_t fd, in_addr_t rem_ip, uint16_t rem_port)
{
  p_tcp_remote_client_t rv = NEW_tcp_remote_client();
  if ( rv ) {
    memset ( rv, 0, SIZE_tcp_remote_client );
    rv->fd = fd;
    rv->remote_ip = rem_ip;
    strcpy ( rv->remote_ip_str, inet_ntoa ( *( (struct in_addr*)(void*) &rem_ip ) ) );
    rv->remote_port = rem_port;
    rv->io_task =
      io_sched_create_reader_task ( owner->io_task->owner,
                                    fd, IO_SCHEDULER_NO_TIMEOUT, (void*) rv,
                                    on_tcp_listener_client_request );
    rv->owner = owner;
  }
  return rv;
}

bool_t
tcp_remote_client_start ( p_tcp_remote_client_t remcli )
{
  if ( remcli )
    return io_sched_schedule_task ( remcli->io_task );
  else
    return CMNUTIL_FALSE;
}

void
tcp_remote_client_stop ( p_tcp_remote_client_t remcli )
{
  if ( remcli ) {
    io_sched_unschedule_task ( remcli->io_task );
    remcli->io_task = NIL_IO_SCHEDULER_TASK;
  }
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

static void
on_tcp_client_connected_to_server ( p_io_scheduler_t scheduler, sock_fd_t fd, int err, void * userdata )
{
  p_tcp_client_t client = AS_PTR_tcp_client( userdata );
  
  assert ( client );
  
  if ( err ) {
    LOGSVC_DEBUG( "on_tcp_client_connected_to_server(): received error code: %d", err );
    return;
  }
  
  LOGSVC_INFO( "Connected to '%s:%d'", client->remote_ip_str, client->remote_port );
  if ( client->on_connected )
    client->on_connected ( client );
  
  // Start up the response handler for the connection.
  if ( !( tcp_client_start ( client, scheduler ) ) ) {
    LOGSVC_ERROR( "Failed to start I/O handler for client connected to '%s:%d'; disconnecting.",
                  client->remote_ip_str, client->remote_port );
    tcp_client_disconnect ( client );
  }
  else {
    LOGSVC_DEBUG( "I/O handler for client connected to '%s:%d' started.", client->remote_ip_str, client->remote_port );
  }
}

static bool_t
on_tcp_client_server_responded ( p_io_scheduler_task_t task, int errcode )
{
  p_tcp_client_t client = AS_PTR_tcp_client( task->user_data );
  int bytes_read;
  
  if ( !( client ) || ( client->fd == INVALID_SOCKET_FD ) ) {
    LOGSVC_DEBUG( "on_tcp_client_server_responded(): client not set or file descriptor invalid." );
    return IO_SCHEDULER_TASK_COMPLETE;
  }
  
  assert ( client->read_buffer != (char*) 0 );
  bytes_read = tcp_receive ( client->fd, client->read_buffer, client->read_buffer_size );
  
  if ( bytes_read <= 0 ) {
    if ( bytes_read < 0 ) {
      if ( ( errno == EAGAIN ) || ( errno == EINTR ) )
        return IO_SCHEDULER_TASK_INCOMPLETE;
      LOGSVC_ERROR( "on_tcp_client_server_responded(): Failed to read from server: %s", strerror ( errno ) );
    }
    else {
      LOGSVC_INFO( "Server '%s:%d' disconnected.", client->remote_ip_str, client->remote_port );
    }
    client->io_task = NIL_IO_SCHEDULER_TASK;
    close ( client->fd );
    client->fd = INVALID_SOCKET_FD;
    if ( client->on_closed )
      client->on_closed ( client, TCP_CLIENT_CLOSED_REMOTE );
    return IO_SCHEDULER_TASK_COMPLETE;
  }
  
  if ( client->on_server_responded &&
       client->on_server_responded ( client, client->read_buffer, (size_t) bytes_read ) )
  {
    // Server response indicated that the connection/conversation has terminated; close the socket.
    tcp_client_disconnect ( client );
    return IO_SCHEDULER_TASK_COMPLETE;
  }
  
  return IO_SCHEDULER_TASK_INCOMPLETE;
}

static bool_t
on_tcp_listener_client_request ( p_io_scheduler_task_t task, int errcode )
{
  p_tcp_remote_client_t remcli = AS_PTR_tcp_remote_client( task->user_data );
  p_tcp_listener_t listener;
  int bytes_read;
  
  if ( !( remcli ) || ( remcli->fd == INVALID_SOCKET_FD ) )
    return IO_SCHEDULER_TASK_COMPLETE;
  
  listener = remcli->owner;
  
  if ( !( listener ) || ( listener->fd == INVALID_SOCKET_FD ) )
    return IO_SCHEDULER_TASK_COMPLETE;
  
  assert ( remcli->read_buffer != (char*) 0 );
  bytes_read = tcp_receive ( remcli->fd, remcli->read_buffer, remcli->read_buffer_size );
  
  if ( bytes_read <= 0 ) {
    // An error occurred, or the remote client closed the connection.
    if ( bytes_read < 0 ) {
      if ( ( errno == EAGAIN ) || ( errno == EINTR ) )
        return IO_SCHEDULER_TASK_INCOMPLETE;
      LOGSVC_ERROR( "on_tcp_listener_client_request(): Failed to read from client: %s", strerror ( errno ) );
    }
    else {
      LOGSVC_INFO( "Client '%s:%d' disconnected.", remcli->remote_ip_str, remcli->remote_port );
    }
    if ( listener->on_client_disconnected )
      listener->on_client_disconnected ( listener, remcli );
    tcp_listener_drop_client ( listener, remcli );
    return IO_SCHEDULER_TASK_COMPLETE;
  }
  
  // If we got here, then we successfully read something from the remote client.
  if ( listener->on_client_request &&
       listener->on_client_request ( listener, remcli, remcli->read_buffer, (size_t) bytes_read ) )
  {
    // The client request resulted in the transaction being "completed". Disconnect the client.
    //
    if ( listener->on_client_disconnected )
      listener->on_client_disconnected ( listener, remcli );
    tcp_listener_drop_client ( listener, remcli );
    return IO_SCHEDULER_TASK_COMPLETE;
  }
  
  return IO_SCHEDULER_TASK_INCOMPLETE;
}

static bool_t
on_tcp_listener_client_waiting ( p_io_scheduler_task_t task, int errcode )
{
  p_tcp_listener_t listener = AS_PTR_tcp_listener( task->user_data );
  
  if ( !( listener ) || ( listener->fd == INVALID_SOCKET_FD ) )
    return IO_SCHEDULER_TASK_COMPLETE;
  
  // Call "client waiting" callback and see if we should accept the new client.
  //
  if ( listener->on_client_waiting && !( listener->on_client_waiting ( listener ) ) )
    return IO_SCHEDULER_TASK_INCOMPLETE;
  
  // If no "client waiting" callback was specified, or if one was specified and
  // returned TRUE, we can accept the connection and create the tcp_remote_client
  // instance.
  //
  sock_fd_t fd;
  in_addr_t remip;
  uint16_t remport;
  fd = tcp_accept_full ( listener->fd, &remip, &remport );
  if ( fd != INVALID_SOCKET_FD ) {
    // Successfully accepted the client connection. Add to the remote clients list.
    // The on_client_connected callback should be set, or the client's I/O task will
    // not be set, effectively providing no actual service to the client. If left
    // NULL, we simply want to log the issue, close the client socket and return.
    //
    if ( !( listener->on_client_connected ) ) {
      LOGSVC_NOTICE( "on_tcp_listener_client_waiting(): Listener's on_client_connected callback not set?! Closing remote socket." );
      close ( fd );
      return IO_SCHEDULER_TASK_INCOMPLETE;
    }
    
    // Create a remote client instance that we will add to our list of clients.
    //
    p_tcp_remote_client_t remcli = tcp_remote_client_init ( listener, fd, remip, remport );
    if ( !( remcli ) ) {
      LOGSVC_ERROR( "on_tcp_listener_client_waiting(): Failed to create remote client instance; closing remote socket." );
      close ( fd );
      return IO_SCHEDULER_TASK_INCOMPLETE;
    }
    
    listener->on_client_connected ( listener, remcli );
    
    // If the I/O task is not set, then we assume that the client was handled to
    // completion (successfully or otherwise) in the on_client_connected callback.
    //
    if ( !( remcli->io_task ) ) {
      LOGSVC_DEBUG( "on_tcp_listener_client_waiting(): Remote client's I/O task not set; closing remote socket." );
      tcp_remote_client_destroy ( remcli );
    }
    else {
      p_tcp_remote_client_t last_elem;
      
      // Circular list, appending to "end" of list.
      LOCK_MUTEX( listener->clients_list_mutex );
      last_elem = listener->clients->prev;
      last_elem->next = remcli;               // LAST->N = NEW
      remcli->prev = last_elem;               // NEW->P = LAST
      remcli->next = listener->clients;       // NEW->N = HEAD
      listener->clients->prev = remcli;       // HEAD->P = NEW
      UNLOCK_MUTEX( listener->clients_list_mutex );
      
      if ( !( tcp_remote_client_start ( remcli ) ) ) {
        LOGSVC_ERROR( "on_tcp_listener_client_waiting(): Failed to start client's I/O task; closing remote socket." );
        tcp_listener_drop_client ( listener, remcli );
      }
      else {
        LOGSVC_DEBUG( "on_tcp_listener_client_waiting(): Remote client %s:%d started.",
                      remcli->remote_ip_str, remcli->remote_port );
      }
    }
  }
  
  return IO_SCHEDULER_TASK_INCOMPLETE;
}

static void
tcp_listener_drop_client ( p_tcp_listener_t listener, p_tcp_remote_client_t remcli )
{
  assert ( listener );
  assert ( remcli );
  assert ( remcli->prev );
  assert ( remcli->next );
  
  // Unlink the remote client instance from the listeners list of clients.
  LOCK_MUTEX( listener->clients_list_mutex );
  remcli->prev->next = remcli->next;
  remcli->next->prev = remcli->prev;
  UNLOCK_MUTEX( listener->clients_list_mutex );
  
  tcp_remote_client_destroy ( remcli );
  
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
