
#include "mem_pool.h"

struct _mem_pool
{
  uint8_t * data_buffer;
  uint8_t * * available_blocks;
  off_t next_free_block;
  size_t total_blocks;
  size_t block_size;
};
#define MEM_POOL_STRUCT_SIZE                 (sizeof( struct _mem_pool ))

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

void
mem_pool_destroy ( p_mem_pool pool )
{
  if ( pool ) {
    free ( pool->available_blocks );
    free ( pool->data_buffer );
    free ( pool );
  }
}

void
mem_pool_free ( p_mem_pool pool, void * data_unit )
{
  if ( !(pool) || !(data_unit) )
    return;
  if ( pool->next_free_block > 0 ) {
    pool->next_free_block--;
    pool->available_blocks[ pool->next_free_block ] = (uint8_t*) data_unit;
  }
}

void
mem_pool_free_r ( pthread_mutex_t mutex, p_mem_pool pool, void * data_unit )
{
  LOCK_MUTEX( mutex );
  mem_pool_free ( pool, data_unit );
  UNLOCK_MUTEX( mutex );
}

void *
mem_pool_malloc ( p_mem_pool pool, size_t num_bytes )
{
  void * rv = NULL;
  if ( !(pool) || ( num_bytes > pool->block_size ) )
    return NULL;
  if ( pool->next_free_block < pool->total_blocks ) {
    rv = pool->available_blocks[ pool->next_free_block ];
    memset ( rv, 0, pool->block_size );
    pool->next_free_block++;
  }
  return rv;
}

void *
mem_pool_malloc_r ( pthread_mutex_t mutex, p_mem_pool pool, size_t num_bytes )
{
  void * rv;
  LOCK_MUTEX( mutex );
  rv = mem_pool_malloc ( pool, num_bytes );
  UNLOCK_MUTEX( mutex );
  return rv;
}

p_mem_pool
mem_pool_new ( size_t unit_size, size_t max_units )
{
  off_t idx;
  int padding;
  p_mem_pool pool = (p_mem_pool) malloc ( MEM_POOL_STRUCT_SIZE );
  if ( pool ) {
    memset ( pool, 0, MEM_POOL_STRUCT_SIZE );
    /* We want our data buffer to be 32-bit aligned, so we check our unit size. */
    padding = ( unit_size & 3 );
    if ( padding )
      unit_size += ( 4 - padding );
    pool->block_size = unit_size;
    pool->total_blocks = max_units;
    pool->data_buffer = (uint8_t*) malloc ( unit_size * max_units );
    if ( !(pool->data_buffer) ) {
      free ( pool );
      return (p_mem_pool) 0;
    }
    pool->available_blocks = (uint8_t**) malloc ( max_units * sizeof ( uint8_t* ) );
    if ( !(pool->available_blocks) ) {
      free ( pool->data_buffer );
      free ( pool );
      return (p_mem_pool) 0;
    }
    pool->available_blocks[0] = pool->data_buffer;
    for ( idx = 1; idx < max_units; idx++ ) {
      pool->available_blocks[idx] = pool->available_blocks[idx-1] + unit_size;
    }
    pool->next_free_block = 0;
  }
  return pool;
}
