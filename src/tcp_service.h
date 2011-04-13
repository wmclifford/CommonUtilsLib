/**
 * @file    tcp_service.h
 * @author  William Clifford
 * @brief Provides support for "TCP services," that is, network services using the TCP protocol.
 *
 * Since this is a common idiom, we provide support for network services that either provide a particular service
 * to other applications (servers) or make use of such a service (clients). Rather than have to place a lot of
 * boilerplate code all over the place, a rather generic set of routines is provided here, broken down as follows:
 *
 * <dl>
 *
 * <dt>TCP listeners</dt>
 * <dd>
 * Each listener is configured to open a bound TCP socket on a given port and listen for incoming connections. Each
 * incoming connection is stored in a list of "clients", and handled by an I/O scheduler and various callback routines.
 * These callback routines essentially define the "service" provided by the listener on that particular port.
 * </dd>
 *
 * <dt>TCP clients</dt>
 * <dd>
 * Each client is configured to connect to a remote host, given its IP address and port number. These are not the same
 * as the clients used by the TCP listeners; the TCP listener clients are only used in that particular context. These
 * clients are used to access a service provided by a remote host. They, too, are handled by an I/O scheduler and a
 * number of callback routines.
 * </dd>
 *
 * </dl>
 *
 * Still kicking around the idea of providing an auto-reconnect mechanism for the TCP client connections. This is
 * something that is done frequently in the ICP firmware, and may be worthwhile to make a standard part of this module
 * so that it does not need to be explicitly defined in multiple places - perhaps set a flag in the client structure
 * that would enable the functionality? Will have to see if this is worth doing.
 **/

#ifndef TCP_SERVICE_H__
#define TCP_SERVICE_H__

#include "gccpch.h"

#include "io-scheduler.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Constants  */
/* ---------- */

#define TCP_CLIENT_CLOSED_LOCAL         0x0001
#define TCP_CLIENT_CLOSED_REMOTE        0x0002

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Type definitions and structures  */
/* ---------- ---------- ---------- */

// Forward declarations of structures for use in callback declarations.
struct _tcp_client;
struct _tcp_listener;
struct _tcp_remote_client;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef void ( *tcp_client_closed_t ) ( struct _tcp_client * client, int reason );

typedef void ( *tcp_client_connected_t ) ( struct _tcp_client * client );

typedef bool_t ( *tcp_client_server_responded_t ) ( struct _tcp_client * client, char * response, size_t response_len );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Callback invoked when a remote client connects to an exposed TCP listener.
 * @param listener The tcp_listener instance that received a remote connection.
 * @param client The tcp_remote_client instance associated with the remote connection.
 **/
typedef void ( *tcp_listener_client_connected_t ) ( struct _tcp_listener * listener, struct _tcp_remote_client * client );

/**
 * @brief Callback invoked when a remote client disconnected from the TCP listener.
 * @param listener The tcp_listener instance owning the remote connection.
 * @param client The tcp_remote_client instance that disconnected.
 **/
typedef void ( *tcp_listener_client_disconnected_t ) ( struct _tcp_listener * listener, struct _tcp_remote_client * client );

/**
 * @brief Callback invoked when a remote client makes a request (data read from the remote client).
 * @param listener The tcp_listener instance owning the remote connection.
 * @param client The tcp_remote_client instance that sent the request.
 * @param request_contents The request data.
 * @param request_length The length of the request data.
 * @return True when remote client has completed its transaction and can be closed; otherwise, false.
 **/
typedef bool_t ( *tcp_listener_client_request_t ) ( struct _tcp_listener * listener, struct _tcp_remote_client * client,
                                                    char * request_contents, size_t request_length );

/**
 * @brief Callback invoked when a listening socket has a remote connection waiting.
 * @param listener The tcp_listener instance that has a remote client attempting to connect.
 * @return Returning true will allow the remote connection; returning false will deny the remote client.
 **/
typedef bool_t ( *tcp_listener_client_waiting_t ) ( struct _tcp_listener * listener );

/**
 * @brief Callback invoked when the TCP listening socket has closed.
 * @param listener The tcp_listener instance that was shutdown and its socket closed.
 * @note  This is generally called when the instance is destroyed. It -may- be called elsewhere in the case where the
 *        socket was closed due to an error, but in such conditions, it may be better off to simply destroy the instance
 *        completely and reconstruct it again (depending upon the situation and requirements of the service).
 **/
typedef void ( *tcp_listener_closed_t ) ( struct _tcp_listener * listener );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////
////  tcp_listener
////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _tcp_listener {

  struct _tcp_listener *                next;
  struct _tcp_listener *                prev;
  
  uint16_t                              port;
  sock_fd_t                             fd;
  p_io_scheduler_task_t                 io_task;
  void *                                user_data;
  
  struct _tcp_remote_client *           clients;
  pthread_mutex_t                       clients_list_mutex;
  
  /* Callbacks */
  tcp_listener_client_connected_t       on_client_connected;
  tcp_listener_client_disconnected_t    on_client_disconnected;
  tcp_listener_client_request_t         on_client_request;
  tcp_listener_client_waiting_t         on_client_waiting;
  tcp_listener_closed_t                 on_closed;
  
} tcp_listener_t, * p_tcp_listener_t;

#define SIZE_tcp_listener               (sizeof( struct _tcp_listener ))
#define NEW_tcp_listener()              ( (p_tcp_listener_t) malloc ( sizeof( struct _tcp_listener ) ) )
#define NIL_tcp_listener                ( (p_tcp_listener_t) 0 )
#define AS_PTR_tcp_listener(vp)         ( (p_tcp_listener_t) vp )

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////
////  tcp_remote_client
////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _tcp_remote_client {

  struct _tcp_remote_client *         next;
  struct _tcp_remote_client *         prev;
  
  sock_fd_t                           fd;
  
  /* Remote host IPv4 information */
  in_addr_t                           remote_ip;          /**< @brief Client IPv4 address.                          **/
  char                                remote_ip_str[16];  /**< @brief Client IPv4 address as a string.              **/
  uint16_t                            remote_port;        /**< @brief Client remote port number.                    **/
  
  p_io_scheduler_task_t               io_task;            /**< @brief I/O scheduler task handling the client.       **/
  char *                              read_buffer;        /**< @brief Buffer used for incoming client requests.     **/
  size_t                              read_buffer_size;   /**< @brief Size of read buffer.                          **/
  void *                              user_data;          /**< @brief Generic data buffer; application-specific.    **/
  
  p_tcp_listener_t                    owner;
  
} tcp_remote_client_t, * p_tcp_remote_client_t;

#define SIZE_tcp_remote_client          (sizeof( struct _tcp_remote_client ))
#define NEW_tcp_remote_client()         ( (p_tcp_remote_client_t) malloc ( sizeof( struct _tcp_remote_client ) ) )
#define NIL_tcp_remote_client           ( (p_tcp_remote_client_t) 0 )
#define AS_PTR_tcp_remote_client(vp)    ( (p_tcp_remote_client_t) vp )

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////
////  tcp_client
////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct _tcp_client {
  
  struct _tcp_client *                next;
  struct _tcp_client *                prev;
  
  sock_fd_t                           fd;
  
  /* Remote host IPv4 information */
  in_addr_t                           remote_ip;
  char                                remote_ip_str[16];
  uint16_t                            remote_port;
  
  p_io_scheduler_task_t               io_task;
  char *                              read_buffer;
  size_t                              read_buffer_size;
  void *                              user_data;
  
  // Built-in auto-reconnect??
  //bool_t                              reconnect_automatically;
  //int                                 reconnect_delay;
  
  /* Callbacks */
  tcp_client_closed_t                 on_closed;
  tcp_client_connected_t              on_connected;
  tcp_client_server_responded_t       on_server_responded;
  
} tcp_client_t, * p_tcp_client_t;

#define SIZE_tcp_client                 (sizeof( struct _tcp_client ))
#define NEW_tcp_client()                ( (p_tcp_client_t) malloc ( sizeof( struct _tcp_client ) ) )
#define NIL_tcp_client                  ( (p_tcp_client_t) 0 )
#define AS_PTR_tcp_client(vp)           ( (p_tcp_client_t) vp )

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  tcp_client
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Starts an asynchronous connect to the remote host identified in the given tcp_client instance.
 * @param client The tcp_client instance.
 * @param scheduler An I/O scheduler used for scheduling a callback invoked when the connection completes.
 * @return Upon successful start of the asynchronous connect, true; otherwise, false.
 * @note  This does not guarantee that the connection will be completed by the time this function returns.
 *        Also, when the connection does complete successfully, any additional configuration of the socket (for
 *        example, keepalive packets, timeout, non-blocking, etc.) should be done in the on_connected callback
 *        specified in the tcp_client instance.
 *
 **/
bool_t tcp_client_connect ( p_tcp_client_t client, p_io_scheduler_t scheduler );

/**
 * @brief Cleans up the tcp_client instance and deletes it.
 * @param client The tcp_client instance.
 * @note  If the client is still connected and/or handling communications with the server, the I/O task is
 *        unscheduled and the socket closed before the instance is destroyed.
 *
 **/
void tcp_client_destroy ( p_tcp_client_t client );

/**
 * @brief Disconnects the tcp_client instance from the remote host to which it is connected.
 * @param client The tcp_client instance.
 *
 **/
void tcp_client_disconnect ( p_tcp_client_t client );

/**
 * @brief Creates a new tcp_client instance.
 * @param rem_ip_str The IPv4 address of the remote host as a string.
 * @param rem_port The port to connect to on the remote host.
 * @param buffer_size The size in bytes of the read buffer for handling server responses.
 * @param client_userdata Application-specific data to be stored in the tcp_client instance.
 * @return The new tcp_client instance on success, or NIL_tcp_client on error.
 **/
p_tcp_client_t tcp_client_init ( const char * rem_ip_str, uint16_t rem_port, size_t buffer_size, void * client_userdata );

/**
 * @brief Kicks off an I/O task that handles reading responses from the remote host.
 * @param client The tcp_client instance.
 * @param scheduler An I/O scheduler that watches for incoming data from the remote host.
 * @return True if successful; otherwise, false.
 **/
bool_t tcp_client_start ( p_tcp_client_t client, p_io_scheduler_t scheduler );

/**
 * @brief Unschedules the I/O task that handles the remote host.
 * @param client The tcp_client instance.
 **/
void tcp_client_stop ( p_tcp_client_t client );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  tcp_listener
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void tcp_listener_destroy ( p_tcp_listener_t listener );
p_tcp_listener_t tcp_listener_init ( uint16_t port, void * listener_userdata );
bool_t tcp_listener_start ( p_tcp_listener_t listener, p_io_scheduler_t scheduler );
void tcp_listener_stop ( p_tcp_listener_t listener );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  tcp_remote_client
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void tcp_remote_client_destroy ( p_tcp_remote_client_t remcli );
p_tcp_remote_client_t tcp_remote_client_init ( p_tcp_listener_t owner, sock_fd_t fd, in_addr_t rem_ip, uint16_t rem_port );
bool_t tcp_remote_client_start ( p_tcp_remote_client_t remcli );
void tcp_remote_client_stop ( p_tcp_remote_client_t remcli );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* TCP_SERVICE_H__ */
