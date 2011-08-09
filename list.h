
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


void   l_insert_node (List * list_p, void * data, Node * after);
Node * l_extruct_node(List * list_p, Node * old);
void   l_delete_node (List * list_p, Node * old);
void * l_extruct_data(List * list_p, Node * old);

#define l_addhead(list_p, node_p)        l_insert_node(list_p, node_p, NULL)
#define l_addnext(list_p, node_p, after_p) l_insert_node(list_p, node_p, after_p)
#define l_addtail(list_p, node_p)        l_insert_node(list_p, node_p, (list_p)->t)

#define Stack       List
#define l_push      l_addhead
#define l_pop(list_p) l_extruct_data(list_p, (list_p)->t)

#define Queue           List
#define l_enqueue       l_addtail
#define l_dequeue(list_p) l_extruct_data(list_p, (list_p)->h)


#define FOR_EACH_NODE(list, node_p) \
  for(node_p=(list).h; node_p; node_p=node_p->n)
