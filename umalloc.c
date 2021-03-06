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
morecore(uint nu)
{
    char *p;
    Header *hp;

    if(nu < 4096)
        nu = 4096;
    p = sbrk(nu * sizeof(Header));
    if(p == (char*)-1)
        return 0;
    hp = (Header*)p;
    hp->s.size = nu;
    free((void*)(hp + 1));
    return freep;
}


static Header*
protect_morecore(uint nu)
{
    char *p;
    Header *hp;

    p = sbrk(nu * sizeof(Header));
    if(p == (char*)-1)
        return 0;
    hp = (Header*)p;
    hp->s.size = nu;
    free((void*)(hp + 1));
    return freep;
}




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
            turn_off_p_flag((void*)(p+1));
            return (void*)(p + 1);
        }
        if(p == freep)
            if((p = morecore(nunits)) == 0)
                return 0;
    }
}





void*
pmalloc(void)
{
    Header *p, *prevp;
    uint nunits = 512;
    if((prevp = freep) == 0){
        base.s.ptr = freep = prevp = &base;
        base.s.size = 0;
    }

    int enter = 0;

    for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
        if(enter) {
            if(p->s.size >= nunits){
                if(p->s.size == nunits)
                    prevp->s.ptr = p->s.ptr;
                else {
                    p->s.size -= nunits;
                    p += p->s.size;
                    p->s.size = nunits;
                }
                freep = prevp;
                turn_on_p_flag((void*) (p+1));
                return (void*)(p + 1);
            }
        }

        if(p == freep) {
            if ((p = protect_morecore(512)) == 0)
                return 0;
            enter = 1;
        }

    }
}





int
protect_page(void* ap){
  if((int)(ap-8) % PGSIZE != 0){

        return -1;
    }

    if (is_p_flag_on(ap)) {
        turn_off_w_flag(ap);
        inc_protected_pg_num();
            return 1;

    }
    return -1;
}


int pfree(void* ap){

    if(!is_p_flag_on(ap)){ // only use on pages alloced by palloc
        return -1;
    }
    if(is_w_flag_off(ap)){ // if page protected turn - unprotect before free - otherwise free will fail
        turn_on_w_flag(ap+8);
    }
    dec_protected_pg_num();
    free(ap);
    return 1;
}
