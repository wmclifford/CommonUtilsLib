/**
 * @file single-link-list.h
 * @brief Singly-linked list implementation.
 * @author William Clifford
 *
 **/

#ifndef SINGLE_LINK_LIST_H__
#define SINGLE_LINK_LIST_H__

/* Include the precompiled header for all the standard library includes and project-wide
   definitions. */
#include "gccpch.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

typedef struct _slink_list_node {
  struct _slink_list_node * next;
} slink_list_node_t;
typedef struct _slink_list_node * p_slink_list_node_t;

typedef int ( *slink_list_node_cmpfn_t ) ( const p_slink_list_node_t node,
					   const void * anydata );

typedef int ( *slink_list_node_sortfn_t ) ( const p_slink_list_node_t node1,
					    const p_slink_list_node_t node2 );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

bool_t slinklst_append ( p_slink_list_node_t * listptr, p_slink_list_node_t node );

p_slink_list_node_t slinklst_find ( p_slink_list_node_t list, slink_list_node_cmpfn_t comparefn, const void * comparedata );

bool_t slinklst_insert_after ( p_slink_list_node_t insert_point, p_slink_list_node_t node );

bool_t slinklst_insert_ordered ( p_slink_list_node_t * listptr, p_slink_list_node_t node,
                                 slink_list_node_sortfn_t sortfn );

bool_t slinklst_prepend ( p_slink_list_node_t * listptr, p_slink_list_node_t node );

bool_t slinklst_remove ( p_slink_list_node_t * listptr, p_slink_list_node_t node );

bool_t slinklst_remove_after ( p_slink_list_node_t remove_point );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// Re-entrant versions (thread-safe)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool_t slinklst_append_r ( pthread_mutex_t mutex, p_slink_list_node_t * listptr, p_slink_list_node_t node );

p_slink_list_node_t slinklst_find_r ( pthread_mutex_t mutex, p_slink_list_node_t list,
                                      slink_list_node_cmpfn_t comparefn, const void * comparedata );

bool_t slinklst_insert_after_r ( pthread_mutex_t mutex, p_slink_list_node_t insert_point, p_slink_list_node_t node );

bool_t slinklst_insert_ordered_r ( pthread_mutex_t mutex,
                                   p_slink_list_node_t * listptr, p_slink_list_node_t node,
                                   slink_list_node_sortfn_t sortfn );

bool_t slinklst_prepend_r ( pthread_mutex_t mutex, p_slink_list_node_t * listptr, p_slink_list_node_t node );

bool_t slinklst_remove_r ( pthread_mutex_t mutex, p_slink_list_node_t * listptr, p_slink_list_node_t node );

bool_t slinklst_remove_after_r ( pthread_mutex_t mutex, p_slink_list_node_t remove_point );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* SINGLE_LINK_LIST_H__ */
