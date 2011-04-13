/**
 * @file    circ-link-list.c
 * @author  William Clifford
 *
 **/

#include "circ-link-list.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Constants  */
/* ---------- */

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  Non-reentrant
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** @brief Locates a node in the circular list, searching along the forward links. **/
p_node_dbl_t
circ_list_find ( p_node_dbl_t startpt, node_find_t searchfn, void * searchdata )
{
  ASSERT_EXIT_NULL( startpt, p_node_dbl_t );
  ASSERT_EXIT_NULL( searchfn, p_node_dbl_t );
  
  p_node_dbl_t rv = startpt->next;
  
  while ( rv && ( rv != startpt ) && !( searchfn ( rv, searchdata ) ) )
    rv = rv->next;
  
  return ( rv == startpt ) ? NIL_node_dbl : rv;
}

/** @brief Inserts a node into the circular list immediately following a given node. **/
void
circ_list_insert_after ( p_node_dbl_t inspt, p_node_dbl_t node )
{
  ASSERT_EXIT_VOID( inspt );
  ASSERT_EXIT_VOID( node );
  
  //
  //  INSPT <===> NEXT    to    INSPT ---> NODE <===> NEXT
  //
  node->next = inspt->next;
  inspt->next->prev = node;
  //
  //  INSPT ---> NODE <===> NEXT  to  INSPT <===> NODE <===> NEXT
  //
  node->prev = inspt;
  inspt->next = node;
  
}

/** @brief Inserts a node into the circular list immediately before a given node. **/
void
circ_list_insert_before ( p_node_dbl_t inspt, p_node_dbl_t node )
{
  ASSERT_EXIT_VOID( inspt );
  ASSERT_EXIT_VOID( node );
  
  //
  //  PREV <===> INSPT    to    PREV <===> NODE <--- INSPT
  //
  node->prev = inspt->prev;
  inspt->prev->next = node;
  //
  //  PREV <===> NODE <--- INSPT  to  PREV <===> NODE <===> INSPT
  //
  node->next = inspt;
  inspt->prev = node;
  
}

/** @brief Inserts a node into the circular list, maintaining a particular order in the nodes. **/
void
circ_list_insert_inorder ( p_node_dbl_t startpt, p_node_dbl_t node, node_compare_t cmpfn )
{
  ASSERT_EXIT_VOID( startpt );
  ASSERT_EXIT_VOID( node );
  ASSERT_EXIT_VOID( cmpfn );
  
  p_node_dbl_t nn = startpt->next;
  
  while ( nn && ( nn != startpt ) && ( cmpfn ( nn, node ) <= 0 ) )
    nn = nn->next;
  
  // Insert before nn. This will ensure that items added which are of "equivalent" value to another
  // item already in the list will be inserted in the order in which they were added, and that if
  // we reach the start point (usually the "head" of the list), it has the same effect as adding to
  // the end of the list.
  node->prev = nn->prev;
  nn->prev->next = node;
  node->next = nn;
  nn->prev = node;
}

/** @brief Locates a node in the circular list, searching along the back links. **/
p_node_dbl_t
circ_list_rfind ( p_node_dbl_t startpt, node_find_t searchfn, void * searchdata )
{
  ASSERT_EXIT_NULL( startpt, p_node_dbl_t );
  ASSERT_EXIT_NULL( searchfn, p_node_dbl_t );
  
  p_node_dbl_t rv = startpt->prev;
  
  while ( rv && ( rv != startpt ) && !( searchfn ( rv, searchdata ) ) )
    rv = rv->prev;
  
  return ( rv == startpt ) ? NIL_node_dbl : rv;
}

/** @brief Removes a node from the circular list (does not delete the node). **/
void
circ_list_unlink ( p_node_dbl_t node )
{
  ASSERT_EXIT_VOID( node );
  if ( ( node->prev->next == node ) && ( node->next->prev == node ) ) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = node;
    node->prev = node;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  Thread-safe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////  These are included to provide a way to lock the list before performing any actions on it that may
////  modify its contents. Generally, for single-threaded applications, this is unnecessary, and the
////  non-reentrant versions of the functions should be used instead. Using these will lock/unlock a mutex
////  and will not be as efficient as their counterparts above.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** @brief Locates a node in the circular list, searching along the forward links. **/
p_node_dbl_t
circ_list_find_r ( pthread_mutex_t * mtx, p_node_dbl_t startpt, node_find_t searchfn, void * searchdata )
{
  ASSERT_EXIT_NULL( startpt, p_node_dbl_t );
  ASSERT_EXIT_NULL( searchfn, p_node_dbl_t );
  ASSERT_EXIT_NULL( mtx, p_node_dbl_t );
  
  LOCK_MUTEX_PTR( mtx );
  
  p_node_dbl_t rv = startpt->next;
  
  while ( rv && ( rv != startpt ) && !( searchfn ( rv, searchdata ) ) )
    rv = rv->next;
  
  UNLOCK_MUTEX_PTR( mtx );
  
  return ( rv == startpt ) ? NIL_node_dbl : rv;
}

/** @brief Inserts a node into the circular list immediately following a given node. **/
void
circ_list_insert_after_r ( pthread_mutex_t * mtx, p_node_dbl_t inspt, p_node_dbl_t node )
{
  ASSERT_EXIT_VOID( inspt );
  ASSERT_EXIT_VOID( node );
  ASSERT_EXIT_VOID( mtx );
  
  LOCK_MUTEX_PTR( mtx );
  
  //
  //  INSPT <===> NEXT    to    INSPT ---> NODE <===> NEXT
  //
  node->next = inspt->next;
  inspt->next->prev = node;
  //
  //  INSPT ---> NODE <===> NEXT  to  INSPT <===> NODE <===> NEXT
  //
  node->prev = inspt;
  inspt->next = node;
  
  UNLOCK_MUTEX_PTR( mtx );
}

/** @brief Inserts a node into the circular list immediately before a given node. **/
void
circ_list_insert_before_r ( pthread_mutex_t * mtx, p_node_dbl_t inspt, p_node_dbl_t node )
{
  ASSERT_EXIT_VOID( inspt );
  ASSERT_EXIT_VOID( node );
  ASSERT_EXIT_VOID( mtx );
  
  LOCK_MUTEX_PTR( mtx );
  
  //
  //  PREV <===> INSPT    to    PREV <===> NODE <--- INSPT
  //
  node->prev = inspt->prev;
  inspt->prev->next = node;
  //
  //  PREV <===> NODE <--- INSPT  to  PREV <===> NODE <===> INSPT
  //
  node->next = inspt;
  inspt->prev = node;
  
  UNLOCK_MUTEX_PTR( mtx );
}

/** @brief Inserts a node into the circular list, maintaining a particular order in the nodes. **/
void
circ_list_insert_inorder_r ( pthread_mutex_t * mtx, p_node_dbl_t startpt, p_node_dbl_t node, node_compare_t cmpfn )
{
  ASSERT_EXIT_VOID( startpt );
  ASSERT_EXIT_VOID( node );
  ASSERT_EXIT_VOID( cmpfn );
  ASSERT_EXIT_VOID( mtx );
  
  LOCK_MUTEX_PTR( mtx );
  
  p_node_dbl_t nn = startpt->next;
  
  while ( nn && ( nn != startpt ) && ( cmpfn ( nn, node ) <= 0 ) )
    nn = nn->next;
  
  // Insert before nn. This will ensure that items added which are of "equivalent" value to another
  // item already in the list will be inserted in the order in which they were added, and that if
  // we reach the start point (usually the "head" of the list), it has the same effect as adding to
  // the end of the list.
  node->prev = nn->prev;
  nn->prev->next = node;
  node->next = nn;
  nn->prev = node;
  
  UNLOCK_MUTEX_PTR( mtx );
}

/** @brief Locates a node in the circular list, searching along the back links. **/
p_node_dbl_t
circ_list_rfind_r ( pthread_mutex_t * mtx, p_node_dbl_t startpt, node_find_t searchfn, void * searchdata )
{
  ASSERT_EXIT_NULL( startpt, p_node_dbl_t );
  ASSERT_EXIT_NULL( searchfn, p_node_dbl_t );
  ASSERT_EXIT_NULL( mtx, p_node_dbl_t );
  
  LOCK_MUTEX_PTR( mtx );
  
  p_node_dbl_t rv = startpt->prev;
  
  while ( rv && ( rv != startpt ) && !( searchfn ( rv, searchdata ) ) )
    rv = rv->prev;
  
  UNLOCK_MUTEX_PTR( mtx );
  
  return ( rv == startpt ) ? NIL_node_dbl : rv;
}

/** @brief Removes a node from the circular list (does not delete the node). **/
void
circ_list_unlink_r ( pthread_mutex_t * mtx, p_node_dbl_t node )
{
  ASSERT_EXIT_VOID( node );
  ASSERT_EXIT_VOID( mtx );
  LOCK_MUTEX_PTR( mtx );
  if ( ( node->prev->next == node ) && ( node->next->prev == node ) ) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = node;
    node->prev = node;
  }
  UNLOCK_MUTEX_PTR( mtx );
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Local functions       */
/* ---------- ---------- */

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
