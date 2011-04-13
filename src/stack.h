/**
 * @file    stack.h
 * @author  William Clifford
 * 
 **/

#ifndef STACK_H__
#define STACK_H__

/* Include the precompiled header for all the standard library includes and project-wide
   definitions. */
#include "gccpch.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#define STACK_ERROR_FULL                ((ssize_t) 0xffffffff)
#define STACK_ERROR_MEMORY              ((ssize_t) 0xfffffffe)
#define STACK_ERROR_INVALID_TYPE        ((ssize_t) 0xfffffffd)

#define STACK_UNLIMITED                 ((size_t) 0xffffffff)

struct _stack;
typedef struct _stack * p_stack_t;

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

/* ---------- ---------- ---------- */
/* Fixed-size (limited) stack.      */
/* ---------- ---------- ---------- */
void stack_fixed_clear ( p_stack_t stack );
void stack_fixed_destroy ( p_stack_t stack );
p_stack_t stack_fixed_init ( size_t max_stack_size );
bool_t stack_fixed_is_empty ( p_stack_t stack );
bool_t stack_fixed_is_full ( p_stack_t stack );
void stack_fixed_pop ( p_stack_t stack );
void * stack_fixed_pop_and_return ( p_stack_t stack );
ssize_t stack_fixed_push ( p_stack_t stack, void * data );
size_t stack_fixed_size ( p_stack_t stack );
void * stack_fixed_top ( p_stack_t stack );

/* Re-entrant versions              */
void stack_fixed_clear_r ( pthread_mutex_t mutex, p_stack_t stack );
void stack_fixed_destroy_r ( pthread_mutex_t mutex, p_stack_t stack );
bool_t stack_fixed_is_empty_r ( pthread_mutex_t mutex, p_stack_t stack );
bool_t stack_fixed_is_full_r ( pthread_mutex_t mutex, p_stack_t stack );
void stack_fixed_pop_r ( pthread_mutex_t mutex, p_stack_t stack );
void * stack_fixed_pop_and_return_r ( pthread_mutex_t mutex, p_stack_t stack );
ssize_t stack_fixed_push_r ( pthread_mutex_t mutex, p_stack_t stack, void * data );
size_t stack_fixed_size_r ( pthread_mutex_t mutex, p_stack_t stack );
void * stack_fixed_top_r ( pthread_mutex_t mutex, p_stack_t stack );

/* ---------- ---------- ---------- */
/* Free (list-based) stack.         */
/* ---------- ---------- ---------- */
void stack_free_clear ( p_stack_t stack );
void stack_free_destroy ( p_stack_t stack );
p_stack_t stack_free_init ( void );
bool_t stack_free_is_empty ( p_stack_t stack );
void stack_free_pop ( p_stack_t stack );
void * stack_free_pop_and_return ( p_stack_t stack );
ssize_t stack_free_push ( p_stack_t stack, void * data );
size_t stack_free_size ( p_stack_t stack );
void * stack_free_top ( p_stack_t stack );

/* Re-entrant versions              */
void stack_free_clear_r ( pthread_mutex_t mutex, p_stack_t stack );
void stack_free_destroy_r ( pthread_mutex_t mutex, p_stack_t stack );
bool_t stack_free_is_empty_r ( pthread_mutex_t mutex, p_stack_t stack );
void stack_free_pop_r ( pthread_mutex_t mutex, p_stack_t stack );
void * stack_free_pop_and_return_r ( pthread_mutex_t mutex, p_stack_t stack );
ssize_t stack_free_push_r ( pthread_mutex_t mutex, p_stack_t stack, void * data );
size_t stack_free_size_r ( pthread_mutex_t mutex, p_stack_t stack );
void * stack_free_top_r ( pthread_mutex_t mutex, p_stack_t stack );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* STACK_H__ */
