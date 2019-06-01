#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"


#define TRUE 1
#define PGSIZE 4096
#define null 0
// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};



typedef struct protected_header{
    int used;
    uint va;
    struct protected_header* next;
} protected_header;





typedef union header Header;

static Header base;
static Header *freep;

void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static Header*
morecore(uint nu, int is_protected) //sharon add this flag
{
  char *p;
  Header *hp;

  if((!is_protected) && (nu < 4096))
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  free((void*)(hp + 1));
  return freep;
}

//static Header*
//protect_morecore(uint nu)
//{
//    char *p;
//    Header *hp;
//    p = sbrk(nu * sizeof(Header));
//    if(p == (char*)-1)
//        return 0;
//    hp = (Header*)p;
//    hp->s.size = nu;
//    free((void*)(hp + 1));
//    return freep;
//}
//


void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = morecore(nunits ,TRUE)) == 0)
        return 0;
  }
}


//void*
//pmalloc(){
//   void* p = protect_morecore(512);
//    turn_on_p_flag(p);
//    return p;
//}

static protected_header* head;


protected_header* find_node_in_list(uint va){
    protected_header* current = head;
    while(current){
        if(current->va == va){
            return current;
        }
        current = current->next;
    }
    return (void *)null;
}


void* pmalloc(){
    protected_header* node;

    protected_header* curr = head;
    protected_header* prev = (void*)null;
    if(!head){
        head = malloc(sizeof(protected_header));
        head->used = 1;
        head->next = (void*)null;
        sbrk (PGSIZE - ((uint)sbrk(1)%PGSIZE +1));
        head->va = (uint)sbrk(PGSIZE);
        node = head;
    } else {

        while(curr && curr->used){
            prev = curr;
            curr = curr->next;
        }
        if(!curr){
            prev->next = malloc(sizeof(protected_header));
            prev->next->next = (void*)null;
            sbrk (PGSIZE - ((uint)sbrk(1)%PGSIZE +1));
            prev->next->va = (uint)sbrk(PGSIZE);
            curr = prev->next;
        }
        curr->used = 1;
        node = curr;
    }

    turn_on_p_flag((void*)node->va);
    turn_on_w_flag((void*)node->va);
    turn_on_prsnt_flag((void*)node->va);
    turn_on_user_flag((void*)node->va);
    return (void*) node->va;
}






int
protect_page(void* ap){
    if((uint)(ap) % PGSIZE != 0){
        return -1;
    }

  if (is_p_flag_on(ap)) {
      inc_protected_pg_num();
      return turn_off_w_flag(ap);
  }
  else{
    return -1;
  }
}


int pfree(void* ap){
  protected_header* node= find_node_in_list((uint)ap);
  if( node == (void *)null){
      return -1;
  }
  if(!is_p_flag_on(ap)){ // only use on pages alloced by palloc
    return -1;
  }
  if(is_w_flag_off(ap)){ // if page protected turn - unprotect before free - otherwise free will fail
    turn_on_w_flag(ap);
  }
  turn_off_p_flag(ap);
  node->used =0;
  dec_protected_pg_num();
  //free(ap);
    //return 0;
    return 1;
}