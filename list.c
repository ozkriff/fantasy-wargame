
// реализация двусвязного списка. + очереди и стеки.

// TODO: пояснить реализацию + примеры использования

typedef struct l_node l_node;
struct l_node {
  l_node * n; // pointer to [n]ext node or NULL
  l_node * p; // pointer to [p]revious node or NULL
  void * d;   // pointer to [d]ata
};

typedef struct l_list l_list;
struct l_list {
  l_node * h; // pointer to first ([h]ead) node
  l_node * t; // pointer to last ([t]ail) node
  long count; // number of nodes in list
};



// Создает новый узел, указывающий на переданные данные,
// и вставляет его в список.
// If after == NULL, then <new> will be added at the head 
// of the list, else it will be added following <after>.
// Данные передаютя по указателю (не копируются!)

l_node * l_insert_node (l_list * list, void * data, l_node * after){
  l_node * new = malloc(sizeof(l_node));
  new->d = data;

  l_node * pnode = after ? after : (l_node*)list ;
  new->n = pnode->n;
  new->p = after;
  pnode->n = new;
  if(new->n)  new->n->p=new;  else  list->t=new;
  list->count++;
  return(new);
}



// Вытаскивает узел из списка. 
// Возвращает указатель на узел.
// Не особожает память узла или данных,
// нужно отдельно делать free для данных и узла.

l_node * l_extruct_node (l_list * list, l_node * old){
  if(old){
    if(old->n)  old->n->p=old->p;  else  list->t=old->p;
    if(old->p)  old->p->n=old->n;  else  list->h=old->n;
    list->count--;
  }
  return(old);
}



// Удалить данные и сам узел.

void l_delete_node (l_list * list, l_node * old){
  l_node * node = l_extruct_node(list, old);
  free(node->d);
  free(node);
}



// вытаскивает узел из списка, удаляет узел, 
// возвращает указатель на данные

void * l_extruct_data (l_list * list, l_node * old){
  l_node * node = l_extruct_node(list, old);
  void * data = node->d;
  free(node);
  return(data);
}



#define l_addhead(list, node)        l_insert_node(list, node, NULL)
#define l_addnext(list, node, after) l_insert_node(list, node, after)
#define l_addtail(list, node)        l_insert_node(list, node, list->t)

#define l_stack     l_list
#define l_push      l_addhead
#define l_pop(list) l_extruct_data(list, list->t)

#define l_queue         l_list
#define l_enqueue       l_addtail
#define l_dequeue(list) l_extruct_data(list, list->h)

#define FOR_EACH_NODE(list, node) \
  for(node=list->h; node; node=node->n)


