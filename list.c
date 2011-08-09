
/* Double-linked list, stack, queue. */

#include <malloc.h>
#include "list.h"


/* Create node <new> that points to <data> and insert 
  this node into list.
  If <after>==NULL, then <new> will be added at the head
  of the list, else it will be added following <after>.
  Only pointer to data is stored, no copying! */
Node *
l_insert_node (List * list, void * data, Node * after){
  Node * pnode;
  Node * new = malloc(sizeof(Node));
  new->d = data;

  pnode = after ? after : (Node*)list ;
  new->n = pnode->n;
  new->p = after;
  pnode->n = new;
  if(new->n)  new->n->p=new;  else  list->t=new;
  list->count++;
  return(new);
}



/* Extructs node from list, returns pointer to this node.
	No memory is freed */
Node *
l_extruct_node (List * list, Node * old){
  if(old){
    if(old->n)  old->n->p=old->p;  else  list->t=old->p;
    if(old->p)  old->p->n=old->n;  else  list->h=old->n;
    list->count--;
  }
  return(old);
}



/* Delete data and node. */
void
l_delete_node (List * list, Node * old){
  Node * node = l_extruct_node(list, old);
  free(node->d);
  free(node);
}



/* Extruct node from list, delete node,
  return pointer to data. */
void *
l_extruct_data (List * list, Node * old){
  Node * node = l_extruct_node(list, old);
  void * data = node->d;
  free(node);
  return(data);
}

