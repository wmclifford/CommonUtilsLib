/**
 * @file    custom-pipes.c
 * @author  William Clifford
 *
 **/

#include "custom-pipes.h"

#define CATEGORY_NAME "custom-pipes"
#include "logging-svc.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Constants  */
/* ---------- */

#define MAX_COMMAND_LENGTH              512
#define MAX_COMMAND_ARGC                20

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

pid_t
my_popen ( fd_t * pin, fd_t * pout, fd_t * perr, const char * fmt, ... )
{
  char cmdtxt[ MAX_COMMAND_LENGTH ];
  char * argv[ MAX_COMMAND_ARGC ];
  int nn, jj, status, cc = 0;
  pipe_fd_t sin[2], sout[2], serr[2];
  pid_t child_pid;
  va_list ap;
  
  memset ( cmdtxt, 0, MAX_COMMAND_LENGTH );
  memset ( argv, 0, sizeof( argv ) );
  
  // Set all the requested file descriptors to INVALID_PIPE_FD.
  if ( pin ) *pin = INVALID_PIPE_FD;
  if ( pout ) *pout = INVALID_PIPE_FD;
  if ( perr ) *perr = INVALID_PIPE_FD;
  
  // Construct the command text.
  va_start ( ap, fmt );
  nn = vsnprintf ( cmdtxt, MAX_COMMAND_LENGTH, fmt, ap );
  va_end ( ap );
  if ( nn < 0 )
    return (pid_t) 0;
  
  // Break out arguments from command text.
  while ( isspace ( cmdtxt[cc] ) )
    cc++;
  for ( jj = cc = 0; cc < nn && jj < 20; ) {
    argv[jj] = &( cmdtxt[cc] );
    if ( cmdtxt[cc] == '"' ) {
      argv[jj]++;
      cc++;
      while ( ( cc < nn ) && ( cmdtxt[cc] != '"' ) )
        cc++;
    }
    else while ( ( cc < nn ) && !( isspace ( cmdtxt[cc] ) ) ) {
      cc++;
    }
    if ( cc < nn )
      cmdtxt[cc++] = 0;
    jj++;
    while ( ( cc < nn ) && isspace ( cmdtxt[cc] ) )
      cc++;
  }
  argv[jj] = 0;
  
  // If we have any arguments, then we received a valid command string ...
  if ( jj > 0 ) {
    // Create the pipes
    if ( pipe ( sin ) == -1 ) {
      return (pid_t) -1;
    }
    if ( pipe ( sout ) == -1 ) {
      close ( sin[0] ); close ( sin[1] ); return (pid_t) -1;
    }
    if ( pipe ( serr ) == -1 ) {
      close ( sin[0] ); close ( sin[1] ); close ( sout[0] ); close ( sout[1] ); return (pid_t) -1;
    }
    
    // Fork ...
    child_pid = fork ();
    switch ( child_pid ) {
    case -1:
      return (pid_t) -1;
    case 0:
      // Child
      // Close unused FDs
      close ( sin[1] ); close ( sout[0] ); close ( serr[0] );
      // Attach pipes to STDIN, STDOUT, STDERR
      if ( dup2 ( sin[0], 0 ) == -1 ) {
        LOGSVC_CRITICAL( "my_popen(): Unable to create STDIN for child process: %s", strerror ( errno ) ); _exit ( 1 );
      }
      if ( dup2 ( sout[1], 1 ) == -1 ) {
        LOGSVC_CRITICAL( "my_popen(): Unable to create STDOUT for child process: %s", strerror ( errno ) ); _exit ( 1 );
      }
      if ( dup2 ( serr[1], 2 ) == -1 ) {
        LOGSVC_CRITICAL( "my_popen(): Unable to create STDERR for child process: %s", strerror ( errno ) ); _exit ( 1 );
      }
      // Close the remaining unused FDs
      close ( sin[0] ); close ( sout[1] ); close ( serr[1] );
      execvp ( argv[0], argv );
      // If here, execution failed, exit so waitpid() can detect error
      _exit ( 1 );
      break;
    default:
      // Parent
      // Close unused FDs
      close ( sin[0] ); close ( sout[1] ); close ( serr[1] );
      // See if there was an execution error in the child process (immediately terminated)
      if ( waitpid ( child_pid, &status, WNOHANG ) == -1 ) {
        LOGSVC_ERROR( "my_popen(): Error executing '%s'", argv[0] );
        close ( sin[1] ); close ( sout[0] ); close ( serr[0] );
        return (pid_t) -1;
      }
      // Pass back FDs, if requested; otherwise, close them.
      if ( pin ) { *pin = sout[0]; } else { close ( sout[0] ); }
      if ( pout ) { *pout = sin[1]; } else { close ( sin[1] ); }
      if ( perr ) { *perr = serr[0]; } else { close ( serr[0] ); }
      return child_pid;
    }
    
  }
  
  return (pid_t) -1;
}

char *
my_system ( int * result_length, const char * fmt, ... )
{
  char * rv = (char*) 0;
  char cmdtxt[MAX_COMMAND_LENGTH];
  char * tt;
  int in, nn, loc=0, cb=1024;
  pid_t child_pid;
  va_list ap;
  
  memset ( cmdtxt, 0, MAX_COMMAND_LENGTH );
  
  if ( result_length ) *result_length = 0;
  
  va_start ( ap, fmt );
  strcpy ( cmdtxt, "/bin/sh -c \"" );
  nn = vsnprintf ( cmdtxt+12, MAX_COMMAND_LENGTH-14, fmt, ap );
  strcat ( cmdtxt, "\"" );
  va_end ( ap );
  if ( nn < 0 )
    return (char*) 0;
  
  LOGSVC_DEBUG ( "my_system(): Executing '%s'", cmdtxt );
  child_pid = my_popen ( &in, 0, 0, cmdtxt );
  if ( child_pid == (pid_t) -1 )
    return (char*) 0;
  
  rv = (char*) malloc ( cb );
  while ( (nn = read ( in, cmdtxt, MAX_COMMAND_LENGTH )) > 0 ) { // <-- Don't like this; should have a separate buffer :(
    if ( loc+nn > cb ) {
      cb += 1024;
      tt = (char*) realloc ( rv, cb );
      if ( !( tt ) ) {
        free ( rv );
        rv = (char*) 0;
        break;
      }
      rv = tt;
    }
    memcpy ( rv+loc, cmdtxt, nn );
    loc += nn;
    rv[loc] = 0;
  }
  waitpid ( child_pid, 0, WNOHANG ); // Clean up child process so it doesn't "zombie" on us.
  
  if ( result_length )
    *result_length = loc;
  else {
    free ( rv );        // ???  why do we do this? Need to see if there are any calls to this routine that
    rv = (char*) 0;     //      pass NULL as the first argument, not expecting a return value ...
  }
  
  return rv;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
