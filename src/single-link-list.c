/**
 * @file single-link-list.c
 * @brief Singly-linked list implementation.
 * @author William Clifford
 *
 * Basic single-link list implementation. For thread-safe versions of the functions, use
 * those suffixed with "_r" (for example, the thread-safe versino of the slinklst_find
 * function is slinklst_find_r). To make use of these functions, your data structures must
 * have a pointer as its first element, and this pointer should point to the next element in
 * your list. When passing your structures to these functions, typecast them to variables of
 * type p_slink_list_node_t, much the same way that the socket handling functions work when
 * passing a sockaddr_in structure to a function expecting a sockaddr structure.
 *
 * Example:
 * 
 * <code>
 * 
 * typedef struct _mydata {
 *   struct mydata * next;
 *   int ival;
 *   float fval;
 * } mydata_t;
 * 
 * typedef struct _mydata * p_mydata_t;
 *
 * p_mydata_t my_list = (p_mydata_t) 0;
 *
 * // Somewhere else in your code ...
 * 
 * p_mydata_t insert_after_me;
 * p_mydata_t node_to_insert;
 *
 * // Make sure that the previous two pointers are constructed/initialized properly.
 * // Then ...
 * 
 * slinklst_insert_after ( (p_slink_list_node_t) insert_after_me,
 *                         (p_slink_list_node_t) node_to_insert );
 *
 * </code>
 * 
 **/

#include "single-link-list.h"

#define CATEGORY_NAME "single-link-list"
#include "logging-svc.h"

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
/* Exposed functions     */
/* ---------- ---------- */

bool_t
slinklst_append ( p_slink_list_node_t * listptr, p_slink_list_node_t node )
{
  p_slink_list_node_t pnode;
  
  /* Have to have valid pointers. */
  if ( !listptr || !node )
    return CMNUTIL_FALSE;
  
  /* If the list is empty, we can simply set the node to the head of the list. */
  if ( !(*listptr) ) {
    node->next = *listptr;
    *listptr = node;
    return CMNUTIL_TRUE;
  }
  
  /* Loop to the end of the list, then append the node. */
  pnode = *listptr;
  while ( pnode->next )
    pnode = pnode->next;
  pnode->next = node;
  node->next = (p_slink_list_node_t) 0;
  return CMNUTIL_TRUE;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_append_r ( pthread_mutex_t mutex, p_slink_list_node_t * listptr, p_slink_list_node_t node )
{
  bool_t rv;
  /* Lock the mutex */
  rv = slinklst_append ( listptr, node );
  /* Unlock the mutex */
  return rv;
}

/* ---------- ---------- ---------- ---------- */
p_slink_list_node_t
slinklst_find ( p_slink_list_node_t list,
                slink_list_node_cmpfn_t comparefn, const void * comparedata )
{
  p_slink_list_node_t rv = list;
  while ( rv && ( comparefn ( rv, comparedata ) != 0 ) )
    rv = rv->next;
  return rv;
}

/* ---------- ---------- ---------- ---------- */
p_slink_list_node_t
slinklst_find_r ( pthread_mutex_t mutex, p_slink_list_node_t list,
                  slink_list_node_cmpfn_t comparefn, const void * comparedata )
{
  p_slink_list_node_t rv;
  LOCK_MUTEX( mutex );
  rv = slinklst_find ( list, comparefn, comparedata );
  UNLOCK_MUTEX( mutex );
  return rv;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_insert_after ( p_slink_list_node_t insert_point, p_slink_list_node_t node )
{
  if ( !insert_point || !node )
    return CMNUTIL_FALSE;
  node->next = insert_point->next;
  insert_point->next = node;
  return CMNUTIL_TRUE;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_insert_after_r ( pthread_mutex_t mutex,
                          p_slink_list_node_t insert_point, p_slink_list_node_t node )
{
  if ( !insert_point || !node )
    return CMNUTIL_FALSE;
  LOCK_MUTEX( mutex );
  node->next = insert_point->next;
  insert_point->next = node;
  UNLOCK_MUTEX( mutex );
  return CMNUTIL_TRUE;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_insert_ordered ( p_slink_list_node_t * listptr, p_slink_list_node_t node,
                          slink_list_node_sortfn_t sortfn )
{
  p_slink_list_node_t pnode;
  
  if ( !listptr || !node )
    return CMNUTIL_FALSE;
  
  if ( !(*listptr) ) {
    *listptr = node;
    return CMNUTIL_TRUE;
  }
  
  if ( sortfn ( node, *listptr ) < 0 ) {
    node->next = *listptr;
    *listptr = node;
    return CMNUTIL_TRUE;
  }
  
  pnode = *listptr;
  while ( pnode->next && ( sortfn ( node, pnode->next ) >= 0 ) )
    pnode = pnode->next;
  node->next = pnode->next;
  pnode->next = node;
  return CMNUTIL_TRUE;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_insert_ordered_r ( pthread_mutex_t mutex, p_slink_list_node_t * listptr,
                            p_slink_list_node_t node, slink_list_node_sortfn_t sortfn )
{
  bool_t rv;
  LOCK_MUTEX( mutex );
  rv = slinklst_insert_ordered ( listptr, node, sortfn );
  UNLOCK_MUTEX( mutex );
  return rv;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_prepend ( p_slink_list_node_t * listptr, p_slink_list_node_t node )
{
  if ( !listptr || !node )
    return CMNUTIL_FALSE;
  
  node->next = *listptr;
  *listptr = node;
  return CMNUTIL_TRUE;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_prepend_r ( pthread_mutex_t mutex, p_slink_list_node_t * listptr, p_slink_list_node_t node )
{
  if ( !listptr || !node )
    return CMNUTIL_FALSE;
  
  LOCK_MUTEX( mutex );
  node->next = *listptr;
  *listptr = node;
  UNLOCK_MUTEX( mutex );
  return CMNUTIL_TRUE;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_remove ( p_slink_list_node_t * listptr, p_slink_list_node_t node )
{
  p_slink_list_node_t pnode;
  
  if ( !listptr || !node || !(*listptr) )
    return CMNUTIL_FALSE;
  
  if ( *listptr == node ) {
    *listptr = node->next;
    return CMNUTIL_TRUE;
  }
  
  /* Locate node in list - we can't remove it if it is not in the list! */
  pnode = *listptr;
  while ( pnode->next && ( pnode->next != node ) )
    pnode = pnode->next;
  
  if ( pnode->next == node ) {
    /* Found it */
    pnode->next = node->next;
    return CMNUTIL_TRUE;
  }
  
  /* Didn't find it */
  return CMNUTIL_FALSE;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_remove_r ( pthread_mutex_t mutex,
                    p_slink_list_node_t * listptr, p_slink_list_node_t node )
{
  bool_t rv;
  LOCK_MUTEX( mutex );
  rv = slinklst_remove ( listptr, node );
  UNLOCK_MUTEX( mutex );
  return rv;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_remove_after ( p_slink_list_node_t remove_point )
{
  if ( !remove_point || !(remove_point->next) )
    return CMNUTIL_FALSE;
  remove_point->next = remove_point->next->next;
  return CMNUTIL_TRUE;
}

/* ---------- ---------- ---------- ---------- */
bool_t
slinklst_remove_after_r ( pthread_mutex_t mutex, p_slink_list_node_t remove_point )
{
  bool_t rv = CMNUTIL_FALSE;
  if ( !remove_point )
    return CMNUTIL_FALSE;
  LOCK_MUTEX( mutex );
  if ( remove_point->next ) {
    remove_point->next = remove_point->next->next;
    rv = CMNUTIL_TRUE;
  }
  UNLOCK_MUTEX( mutex );
  return rv;
}

/* ---------- ---------- ---------- ---------- ---------- ---------- ---------- ---------- */
