
// реализация двусвязного списка. списки, очереди и стеки.
// functions return a pointer to the node that was 
// inserted or removed.
// n-next p-prev h-head t-tail rem-remove

typedef struct l_node l_node;
typedef struct l_list l_list;
struct l_node { l_node * n; l_node * p; };
struct l_list { l_node * h; l_node * t; long count; };

#define l_new(L)  l_list (L)[1] = {{NULL, NULL, 0}}

// If after == NULL, then <new> will be added at the head 
// of the list, else it will be added following <after>.
l_node * l_ins(l_list * lst, l_node * new, l_node * after){
  l_node * pnode = after ? after : (l_node*)lst ;
  new->n = pnode->n;
  new->p = after;
  pnode->n = new;
  if(new->n)  new->n->p=new;  else  lst->t=new;
  lst->count++;
  return(new);
}

l_node * l_remv(l_list * lst, l_node * old){
  if(old){
    if(old->n)  old->n->p=old->p;  else  lst->t=old->p;
    if(old->p)  old->p->n=old->n;  else  lst->h=old->n;
    lst->count--;
  }
  return(old);
}

// L-list;  N-node;  A-after(insert N after this node);
#define l__ins_(L,N,A) l_ins((l_list*)L, (l_node*)N, A)
#define l_addh(L,N)    l__ins_( L, N, NULL            )
#define l_addn(L,N,A)  l__ins_( L, N, (l_node*)A      )
#define l_addt(L,N)    l__ins_( L, N, ((l_list*)L)->t )

// double linked list
#define l_remh(L ) l_remv((l_list*)L, ((l_list*)L)->h)
#define l_remt(L ) l_remv((l_list*)L, ((l_list*)L)->t)
#define l_rem(L,N) l_remv((l_list*)L, (l_node*)N     )
#define l_first(L) ((l_list*)L)->h
#define l_last(L)  ((l_list*)L)->t
#define l_next(N)  ((l_node*)N)->n
#define l_prev(N)  ((l_node*)N)->p

// stack
#define l_push l_addh
#define l_pop  l_remh

// queue
#define l_enq  l_addt
#define l_deq  l_remh


