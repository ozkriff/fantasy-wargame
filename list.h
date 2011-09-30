/* See LICENSE file for copyright and license details. */

/* Double-linked list, stack, queue. */

typedef struct Node Node;
struct Node {
  Node * n; /* pointer to [n]ext node or NULL */
  Node * p; /* pointer to [p]revious node or NULL */
  void * d;   /* pointer to [d]ata */
};

typedef struct List List;
struct List {
  Node * h; /* pointer to first ([h]ead) node */
  Node * t; /* pointer to last ([t]ail) node */
  int  count; /* number of nodes in list */
};

void   insert_node (List *list_p, void *data, Node *after);
Node * extruct_node(List *list_p, Node *old);
void   delete_node (List *list_p, Node *old);
void * extruct_data(List *list_p, Node *old);

#define add_node_to_head(list_p, node_p) \
  insert_node(list_p, node_p, NULL)
#define add_node_after(list_p, node_p, after_p) \
  insert_node(list_p, node_p, after_p)
#define add_node_to_tail(list_p, node_p) \
  insert_node(list_p, node_p, (list_p)->t)

#define Stack            List
#define push_node        add_node_to_head
#define pop_node(list_p) extruct_data(list_p, (list_p)->t)

#define Queue            List
#define enq_node         add_node_to_tail
#define deq_node(list_p) extruct_data(list_p, (list_p)->h)

#define FOR_EACH_NODE(list, node_p) \
  for(node_p=(list).h; node_p; node_p=node_p->n)

