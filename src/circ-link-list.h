/**
 * @file    circ-link-list.h
 * @author  William Clifford
 **/

#ifndef CIRC_LINK_LIST_H__
#define CIRC_LINK_LIST_H__

/* Include the precompiled header for all the standard library includes and project-wide definitions. */
#include "gccpch.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Constants  */
/* ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Type definitions and structures  */
/* ---------- ---------- ---------- */

// Simple, doubly-linked node.
typedef struct _node_dbl {
  struct _node_dbl * next;
  struct _node_dbl * prev;
} node_dbl_t, * p_node_dbl_t;

#define INIT_node_dbl(n)                do { (n)->next = (n); (n)->prev = (n); } while ( 0 )
#define NIL_node_dbl                    ( (p_node_dbl_t) 0 )
#define AS_PTR_node_dbl(vp)             ( (p_node_dbl_t) vp ) 

// Returns -1 when x < y, 0 when x == y, and 1 when x > y.
typedef int ( *node_compare_t ) ( p_node_dbl_t x, p_node_dbl_t y );

typedef bool_t ( *node_find_t ) ( p_node_dbl_t, void * );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  Non-reentrant
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** @brief Locates a node in the circular list, searching along the forward links. **/
p_node_dbl_t circ_list_find ( p_node_dbl_t startpt, node_find_t searchfn, void * searchdata );

/** @brief Inserts a node into the circular list immediately following a given node. **/
void circ_list_insert_after ( p_node_dbl_t inspt, p_node_dbl_t node );

/** @brief Inserts a node into the circular list immediately before a given node. **/
void circ_list_insert_before ( p_node_dbl_t inspt, p_node_dbl_t node );

/** @brief Inserts a node into the circular list, maintaining a particular order in the nodes. **/
void circ_list_insert_inorder ( p_node_dbl_t startpt, p_node_dbl_t node, node_compare_t cmpfn );

/** @brief Locates a node in the circular list, searching along the back links. **/
p_node_dbl_t circ_list_rfind ( p_node_dbl_t startpt, node_find_t searchfn, void * searchdata );

/** @brief Removes a node from the circular list (does not delete the node). **/
void circ_list_unlink ( p_node_dbl_t node );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  Thread-safe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  These are included to provide a way to lock the list before performing any actions on it that may
////  modify its contents. Generally, for single-threaded applications, this is unnecessary, and the
////  non-reentrant versions of the functions should be used instead. Using these will lock/unlock a mutex
////  and will not be as efficient as their counterparts above.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** @brief Locates a node in the circular list, searching along the forward links. **/
p_node_dbl_t circ_list_find_r ( pthread_mutex_t * mtx, p_node_dbl_t startpt, node_find_t searchfn, void * searchdata );

/** @brief Inserts a node into the circular list immediately following a given node. **/
void circ_list_insert_after_r ( pthread_mutex_t * mtx, p_node_dbl_t inspt, p_node_dbl_t node );

/** @brief Inserts a node into the circular list immediately before a given node. **/
void circ_list_insert_before_r ( pthread_mutex_t * mtx, p_node_dbl_t inspt, p_node_dbl_t node );

/** @brief Inserts a node into the circular list, maintaining a particular order in the nodes. **/
void circ_list_insert_inorder_r ( pthread_mutex_t * mtx, p_node_dbl_t startpt, p_node_dbl_t node, node_compare_t cmpfn );

/** @brief Locates a node in the circular list, searching along the back links. **/
p_node_dbl_t circ_list_rfind_r ( pthread_mutex_t * mtx, p_node_dbl_t startpt, node_find_t searchfn, void * searchdata );

/** @brief Removes a node from the circular list (does not delete the node). **/
void circ_list_unlink_r ( pthread_mutex_t * mtx, p_node_dbl_t node );

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */

#endif /* CIRC_LINK_LIST_H__ */
