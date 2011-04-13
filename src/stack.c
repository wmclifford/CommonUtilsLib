/**
 * @file stack.c
 * @author William Clifford
 *
 **/

#include "stack.h"

#define CATEGORY_NAME "stack"
#include "logging-svc.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

typedef struct _stack_free_node {
  struct _stack_free_node * next;
  void * data;
} stack_free_node_t;
typedef struct _stack_free_node * p_stack_free_node_t;
#define STACK_FREE_NODE_STRUCT_SIZE     (sizeof( struct _stack_free_node ))

#define STACK_TYPE_FIXED                0x0000
#define STACK_TYPE_FREE                 0x0001

struct _stack {
  int16_t type;
  size_t current;
  size_t limit;
  union {
    p_stack_free_node_t free_top;
    void * * fixed_base;
  };
};
/* Don't need to redefine the p_stack_t type; it's already defined in the header. */
#define STACK_STRUCT_SIZE               (sizeof( struct _stack ))

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Shared (global) variables        */
/* ---------- ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local function prototypes        */
/* ---------- ---------- ---------- */

static inline void inl_stack_fixed_clear ( p_stack_t stack );
static inline void inl_stack_fixed_destroy ( p_stack_t stack );
static inline bool_t inl_stack_fixed_is_empty ( p_stack_t stack );
static inline bool_t inl_stack_fixed_is_full ( p_stack_t stack );
static inline void inl_stack_fixed_pop ( p_stack_t stack );
static inline void * inl_stack_fixed_pop_and_return ( p_stack_t stack );
static inline ssize_t inl_stack_fixed_push ( p_stack_t stack, void * data );
static inline size_t inl_stack_fixed_size ( p_stack_t stack );
static inline void * inl_stack_fixed_top ( p_stack_t stack );

static inline void inl_stack_free_clear ( p_stack_t stack );
static inline void inl_stack_free_destroy ( p_stack_t stack );
static inline bool_t inl_stack_free_is_empty ( p_stack_t stack );
static inline void inl_stack_free_pop ( p_stack_t stack );
static inline void * inl_stack_free_pop_and_return ( p_stack_t stack );
static inline ssize_t inl_stack_free_push ( p_stack_t stack, void * data );
static inline size_t inl_stack_free_size ( p_stack_t stack );
static inline void * inl_stack_free_top ( p_stack_t stack );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Module variables      */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

/* ---------- ---------- ---------- */
/* Fixed-size (limited) stack.      */
/* ---------- ---------- ---------- */
void
stack_fixed_clear ( p_stack_t stack )
{
  inl_stack_fixed_clear ( stack );
}

void
stack_fixed_destroy ( p_stack_t stack )
{
  inl_stack_fixed_destroy ( stack );
}

p_stack_t
stack_fixed_init ( size_t max_stack_size )
{
  p_stack_t rv;
  size_t array_size = max_stack_size * sizeof( void* );
  
  if ( max_stack_size == 0 )
    return (p_stack_t) 0;
  
  rv = (p_stack_t) malloc ( STACK_STRUCT_SIZE );
  if ( rv ) {
    memset ( rv, 0, STACK_STRUCT_SIZE );
    rv->fixed_base = (void**) malloc ( array_size );
    if ( !(rv->fixed_base) ) {
      free ( rv );
      return (p_stack_t) 0;
    }
    memset ( rv->fixed_base, 0, array_size );
    rv->type = STACK_TYPE_FIXED;
    rv->limit = max_stack_size;
    rv->current = rv->limit;
  }
  return rv;
}

bool_t
stack_fixed_is_empty ( p_stack_t stack )
{
  return inl_stack_fixed_is_empty ( stack );
}

bool_t
stack_fixed_is_full ( p_stack_t stack )
{
  return inl_stack_fixed_is_full ( stack );
}

void
stack_fixed_pop ( p_stack_t stack )
{
  inl_stack_fixed_pop ( stack );
}

void *
stack_fixed_pop_and_return ( p_stack_t stack )
{
  return inl_stack_fixed_pop_and_return ( stack );
}

ssize_t
stack_fixed_push ( p_stack_t stack, void * data )
{
  return inl_stack_fixed_push ( stack, data );
}

size_t
stack_fixed_size ( p_stack_t stack )
{
  return inl_stack_fixed_size ( stack );
}

void *
stack_fixed_top ( p_stack_t stack )
{
  return inl_stack_fixed_top ( stack );
}

/* Re-entrant versions              */

void
stack_fixed_clear_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  LOCK_MUTEX( mutex );
  inl_stack_fixed_clear ( stack );
  UNLOCK_MUTEX( mutex );
}

void
stack_fixed_destroy_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  LOCK_MUTEX( mutex );
  inl_stack_fixed_destroy ( stack );
  UNLOCK_MUTEX( mutex );
}

bool_t
stack_fixed_is_empty_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  bool_t rv;
  LOCK_MUTEX( mutex );
  rv = inl_stack_fixed_is_empty ( stack );
  UNLOCK_MUTEX( mutex );
  return rv;
}

bool_t
stack_fixed_is_full_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  bool_t rv;
  LOCK_MUTEX( mutex );
  rv = inl_stack_fixed_is_full ( stack );
  UNLOCK_MUTEX( mutex );
  return rv;
}

void
stack_fixed_pop_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  LOCK_MUTEX( mutex );
  inl_stack_fixed_pop ( stack );
  UNLOCK_MUTEX( mutex );
}

void *
stack_fixed_pop_and_return_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  void * rv = (void*) 0;
  LOCK_MUTEX( mutex );
  rv = inl_stack_fixed_pop_and_return ( stack );
  UNLOCK_MUTEX( mutex );
  return rv;
}

ssize_t
stack_fixed_push_r ( pthread_mutex_t mutex, p_stack_t stack, void * data )
{
  ssize_t rv;
  LOCK_MUTEX( mutex );
  rv = inl_stack_fixed_push ( stack, data );
  UNLOCK_MUTEX( mutex );
  return rv;
}

size_t
stack_fixed_size_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  size_t rv = 0;
  LOCK_MUTEX( mutex );
  rv = inl_stack_fixed_size ( stack );
  UNLOCK_MUTEX( mutex );
  return rv;
}

void *
stack_fixed_top_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  void * rv = (void*) 0;
  LOCK_MUTEX( mutex );
  rv = inl_stack_fixed_top ( stack );
  UNLOCK_MUTEX( mutex );
  return rv;
}

/* ---------- ---------- ---------- */
/* Free (list-based) stack.         */
/* ---------- ---------- ---------- */
void
stack_free_clear ( p_stack_t stack )
{
  inl_stack_free_clear ( stack );
}

void
stack_free_destroy ( p_stack_t stack )
{
  inl_stack_free_destroy ( stack );
}

p_stack_t
stack_free_init ( void )
{
  p_stack_t rv = (p_stack_t) malloc ( STACK_STRUCT_SIZE );
  
  if ( rv ) {
    memset ( rv, 0, STACK_STRUCT_SIZE );
    rv->type = STACK_TYPE_FREE;
    rv->free_top = (p_stack_free_node_t) 0;
    rv->limit = STACK_UNLIMITED;
    rv->current = STACK_UNLIMITED;
  }
  return rv;
}

bool_t
stack_free_is_empty ( p_stack_t stack )
{
  return inl_stack_free_is_empty ( stack );
}

void
stack_free_pop ( p_stack_t stack )
{
  inl_stack_free_pop ( stack );
}

void *
stack_free_pop_and_return ( p_stack_t stack )
{
  return inl_stack_free_pop_and_return ( stack );
}

ssize_t
stack_free_push ( p_stack_t stack, void * data )
{
  return inl_stack_free_push ( stack, data );
}

size_t
stack_free_size ( p_stack_t stack )
{
  return inl_stack_free_size ( stack );
}

void *
stack_free_top ( p_stack_t stack )
{
  return inl_stack_free_top ( stack );
}

/* Re-entrant versions              */
void
stack_free_clear_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  LOCK_MUTEX( mutex );
  inl_stack_free_clear ( stack );
  UNLOCK_MUTEX( mutex );
}

void
stack_free_destroy_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  LOCK_MUTEX( mutex );
  inl_stack_free_destroy ( stack );
  UNLOCK_MUTEX( mutex );
}

bool_t
stack_free_is_empty_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  bool_t rv;
  LOCK_MUTEX( mutex );
  rv = inl_stack_free_is_empty ( stack );
  UNLOCK_MUTEX( mutex );
  return rv;
}

void
stack_free_pop_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  LOCK_MUTEX( mutex );
  inl_stack_free_pop ( stack );
  UNLOCK_MUTEX( mutex );
}

void *
stack_free_pop_and_return_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  void * rv;
  LOCK_MUTEX( mutex );
  rv = inl_stack_free_pop_and_return ( stack );
  UNLOCK_MUTEX( mutex );
  return rv;
}

ssize_t
stack_free_push_r ( pthread_mutex_t mutex, p_stack_t stack, void * data )
{
  ssize_t rv;
  LOCK_MUTEX( mutex );
  rv = inl_stack_free_push ( stack, data );
  UNLOCK_MUTEX( mutex );
  return rv;
}

size_t
stack_free_size_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  size_t rv;
  LOCK_MUTEX( mutex );
  rv = inl_stack_free_size ( stack );
  UNLOCK_MUTEX( mutex );
  return rv;
}

void *
stack_free_top_r ( pthread_mutex_t mutex, p_stack_t stack )
{
  void * rv;
  LOCK_MUTEX( mutex );
  rv = inl_stack_free_top ( stack );
  UNLOCK_MUTEX( mutex );
  return rv;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

/* ---------- ---------- ---------- */
/* Fixed-size (limited) stack.      */
/* ---------- ---------- ---------- */

static inline void
inl_stack_fixed_clear ( p_stack_t stack )
{
  if ( !stack || ( stack->type != STACK_TYPE_FIXED ) )
    return;
  memset ( stack->fixed_base, 0, ( stack->limit * sizeof( void * ) ) );
  stack->current = stack->limit;
}

static inline void
inl_stack_fixed_destroy ( p_stack_t stack )
{
  if ( !stack || ( stack->type != STACK_TYPE_FIXED ) )
    return;
  free ( stack->fixed_base );
  free ( stack );
}

static inline bool_t
inl_stack_fixed_is_empty ( p_stack_t stack )
{
  if ( stack && ( stack->type == STACK_TYPE_FIXED ) )
    return ( stack->current == stack->limit );
  else
    return CMNUTIL_FALSE;
}

static inline bool_t
inl_stack_fixed_is_full ( p_stack_t stack )
{
  if ( stack && ( stack->type == STACK_TYPE_FIXED ) )
    return ( stack->current == 0 );
  else
    return CMNUTIL_FALSE;
}

static inline void
inl_stack_fixed_pop ( p_stack_t stack )
{
  if ( !stack || ( stack->type != STACK_TYPE_FIXED ) )
    return;
  if ( stack->current < stack->limit ) {
    stack->fixed_base[ stack->current ] = (void*) 0;
    stack->current++;
  }
}

static inline void *
inl_stack_fixed_pop_and_return ( p_stack_t stack )
{
  void * rv = (void*) 0;
  if ( !stack || ( stack->type != STACK_TYPE_FIXED ) )
    return (void*) 0;
  if ( stack->current < stack->limit ) {
    rv = stack->fixed_base[ stack->current ];
    stack->fixed_base[ stack->current ] = (void*) 0;
    stack->current++;
  }
  return rv;
}

static inline ssize_t
inl_stack_fixed_push ( p_stack_t stack, void * data )
{
  if ( !stack || ( stack->type != STACK_TYPE_FIXED ) )
    return STACK_ERROR_INVALID_TYPE;
  if ( stack->current > 0 ) {
    stack->current--;
    stack->fixed_base[ stack->current ] = data;
    return stack->current;
  }
  return STACK_ERROR_FULL;
}

static inline size_t
inl_stack_fixed_size ( p_stack_t stack )
{
  if ( !stack || ( stack->type != STACK_TYPE_FIXED ) )
    return 0;
  return ( stack->limit - stack->current );
}

static inline void *
inl_stack_fixed_top ( p_stack_t stack )
{
  if ( !stack || ( stack->type != STACK_TYPE_FIXED ) )
    return (void*) 0;
  if ( stack->current < stack->limit )
    return stack->fixed_base[ stack->current ];
  else
    return (void*) 0;
}

/* ---------- ---------- ---------- */
/* Free (list-based) stack.         */
/* ---------- ---------- ---------- */

static inline void
inl_stack_free_clear ( p_stack_t stack )
{
  p_stack_free_node_t nn;
  
  if ( !stack || ( stack->type != STACK_TYPE_FREE ) )
    return;
  while ( stack->free_top ) {
    nn = stack->free_top->next;
    free ( stack->free_top );
    stack->free_top = nn;
  }
}

static inline void
inl_stack_free_destroy ( p_stack_t stack )
{
  p_stack_free_node_t nn;
  
  if ( !stack || ( stack->type != STACK_TYPE_FREE ) )
    return;
  while ( stack->free_top ) {
    nn = stack->free_top->next;
    free ( stack->free_top );
    stack->free_top = nn;
  }
  free ( stack );
}

static inline bool_t
inl_stack_free_is_empty ( p_stack_t stack )
{
  if ( !stack || ( stack->type != STACK_TYPE_FREE ) )
    return CMNUTIL_FALSE;
  return ( stack->free_top == (p_stack_free_node_t) 0 );
}

static inline void
inl_stack_free_pop ( p_stack_t stack )
{
  p_stack_free_node_t nn;
  
  if ( !stack || ( stack->type != STACK_TYPE_FREE ) )
    return;
  nn = stack->free_top;
  if ( nn ) {
    stack->free_top = nn->next;
    free ( nn );
  }
}

static inline void *
inl_stack_free_pop_and_return ( p_stack_t stack )
{
  p_stack_free_node_t nn;
  void * rv = (void*) 0;
  
  if ( !stack || ( stack->type != STACK_TYPE_FREE ) )
    return (void*) 0;
  nn = stack->free_top;
  if ( nn ) {
    rv = nn->data;
    stack->free_top = nn->next;
    free ( nn );
  }
  return rv;
}

static inline ssize_t
inl_stack_free_push ( p_stack_t stack, void * data )
{
  p_stack_free_node_t nn;
  
  if ( !stack || ( stack->type != STACK_TYPE_FREE ) )
    return STACK_ERROR_INVALID_TYPE;
  nn = (p_stack_free_node_t) malloc ( STACK_FREE_NODE_STRUCT_SIZE );
  if ( !nn )
    return STACK_ERROR_MEMORY;
  nn->data = data;
  nn->next = stack->free_top;
  stack->free_top = nn;
  return 1;
}

static inline size_t
inl_stack_free_size ( p_stack_t stack )
{
  p_stack_free_node_t nn;
  size_t rv = 0;
  
  if ( stack && ( stack->type == STACK_TYPE_FREE ) ) {
    nn = stack->free_top;
    while ( nn ) {
      rv++;
      nn = nn->next;
    }
  }
  return rv;
}

static inline void *
inl_stack_free_top ( p_stack_t stack )
{
  void * rv = (void*) 0;
  if ( stack && ( stack->type == STACK_TYPE_FREE ) && ( stack->free_top ) )
    rv = stack->free_top->data;
  return rv;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
