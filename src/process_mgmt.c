/**
 * @file    process_mgmt.c
 * @author  William Clifford
 *
 **/

#include "process_mgmt.h"

#define CATEGORY_NAME "process-mgmt"
#include "logging-svc.h"

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

bool_t
proc_mgmt_is_pid_alive ( pid_t process_pid )
{
  return ( kill ( process_pid, 0 ) == 0 );
}

bool_t
proc_mgmt_is_process_alive ( const char * process_name, pid_t * p_process_pid )
{
  char buffer[24], pid_file_name[512];
  char * p_linefeed;
  FILE * fp_pid;
  pid_t pid;
  bool_t rv;
  
  if ( STR_IS_NULL_OR_EMPTY ( process_name ) )
    return ICP_FALSE;
  
  sprintf ( pid_file_name, "/var/run/%s.pid", process_name );
  fp_pid = fopen ( pid_file_name, "r" );
  if ( !( fp_pid ) )
    return ICP_FALSE;
  
  if ( !( fgets ( buffer, sizeof( buffer ), fp_pid ) ) ) {
    fclose ( fp_pid );
    return ICP_FALSE;
  }
  
  fclose ( fp_pid );
  
  p_linefeed = strrchr ( buffer, '\n' );
  if ( p_linefeed )
    *p_linefeed = 0;
  
  pid = (pid_t) atoi ( buffer );
  rv = ( kill ( pid, 0 ) == 0 );
  
  if ( rv && p_process_pid )
    *p_process_pid = pid;
  
  return rv;
}

bool_t
proc_mgmt_record_my_pid ( const char * process_name )
{
  char pid_file_name[512];
  FILE * fp_pid;
  
  if ( STR_IS_NULL_OR_EMPTY( process_name ) )
    return ICP_FALSE;
  
  sprintf ( pid_file_name, "/var/run/%s.pid", process_name );
  fp_pid = fopen ( pid_file_name, "w" );
  if ( !( fp_pid ) )
    return ICP_FALSE;
  
  fprintf ( fp_pid, "%d", getpid () );
  fflush ( fp_pid );
  fclose ( fp_pid );
  return ICP_TRUE;
}

bool_t
proc_mgmt_record_pid ( const char * process_name, pid_t process_pid )
{
  char pid_file_name[512];
  FILE * fp_pid;
  
  if ( STR_IS_NULL_OR_EMPTY( process_name ) )
    return ICP_FALSE;
  
  sprintf ( pid_file_name, "/var/run/%s.pid", process_name );
  fp_pid = fopen ( pid_file_name, "w" );
  if ( !( fp_pid ) )
    return ICP_FALSE;
  
  fprintf ( fp_pid, "%d", process_pid );
  fflush ( fp_pid );
  fclose ( fp_pid );
  return ICP_TRUE;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
