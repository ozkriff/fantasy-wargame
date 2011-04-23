
/* реализация двусвязного списка. + очереди и стеки. */

/* TODO: пояснить реализацию + примеры использования */

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
  long count; /* number of nodes in list */
};



/* Создает новый узел, указывающий на переданные данные, */
/* и вставляет его в список. */
/* If after == NULL, then <new> will be added at the head  */
/* of the list, else it will be added following <after>. */
/* Данные передаютя по указателю (не копируются!) */

Node * l_insert_node (List * list, void * data, Node * after){
  Node * new = malloc(sizeof(Node));
  new->d = data;

  Node * pnode = after ? after : (Node*)list ;
  new->n = pnode->n;
  new->p = after;
  pnode->n = new;
  if(new->n)  new->n->p=new;  else  list->t=new;
  list->count++;
  return(new);
}



/* Вытаскивает узел из списка.  */
/* Возвращает указатель на узел. */
/* Не особожает память узла или данных, */
/* нужно отдельно делать free для данных и узла. */

Node * l_extruct_node (List * list, Node * old){
  if(old){
    if(old->n)  old->n->p=old->p;  else  list->t=old->p;
    if(old->p)  old->p->n=old->n;  else  list->h=old->n;
    list->count--;
  }
  return(old);
}



/* Удалить данные и сам узел. */

void l_delete_node (List * list, Node * old){
  Node * node = l_extruct_node(list, old);
  free(node->d);
  free(node);
}



/* вытаскивает узел из списка, удаляет узел,  */
/* возвращает указатель на данные */

void * l_extruct_data (List * list, Node * old){
  Node * node = l_extruct_node(list, old);
  void * data = node->d;
  free(node);
  return(data);
}



#define l_addhead(list, node)        l_insert_node(list, node, NULL)
#define l_addnext(list, node, after) l_insert_node(list, node, after)
#define l_addtail(list, node)        l_insert_node(list, node, list->t)

#define Stack       List
#define l_push      l_addhead
#define l_pop(list) l_extruct_data(list, list->t)

#define Queue           List
#define l_enqueue       l_addtail
#define l_dequeue(list) l_extruct_data(list, list->h)

#define FOR_EACH_NODE(list, node) \
  for(node=list->h; node; node=node->n)


