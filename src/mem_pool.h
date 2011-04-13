/**
 * @file    mem_pool.h
 * @author  William Clifford
 **/

#ifndef MEM_POOL_H__
#define MEM_POOL_H__

/* Include the precompiled header for all the standard library includes and
   project-wide definitions. */
#include "gccpch.h"

struct _mem_pool;
typedef struct _mem_pool * p_mem_pool;

void mem_pool_destroy ( p_mem_pool pool );

void mem_pool_free ( p_mem_pool pool, void * data_unit );

void mem_pool_free_r ( pthread_mutex_t mutex, p_mem_pool pool, void * data_unit );

void * mem_pool_malloc ( p_mem_pool pool, size_t num_bytes );

void * mem_pool_malloc_r ( pthread_mutex_t mutex, p_mem_pool pool, size_t num_bytes );

p_mem_pool mem_pool_new ( size_t unit_size, size_t max_units );

#endif /* MEM_POOL_H__ */
