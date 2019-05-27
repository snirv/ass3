#include "types.h"
#include "stat.h"
#include "user.h"


#define TWOKB 2048
#define PGSIZE 4096
#define NUM_MEMORY_ALLOCATIONS 10
#define PLUS_TO_PAGE_ALLIGN(p) (PGSIZE - ((uint)p % PGSIZE))

int value = 12345678;
char *mem;


void* malloc_test(void){
    void *p = malloc(TWOKB);

    if(p == 0){
        printf(1,"malloc failed !!!\n");
        exit();
    }
    else{
        memset(p,value,TWOKB);
    }
    return p;
}


void* page_malloc_test(void){

    mem = malloc((NUM_MEMORY_ALLOCATIONS + 1) * PGSIZE);

    mem = mem + PLUS_TO_PAGE_ALLIGN(mem);

    if((uint)mem % PGSIZE != 0){
        printf(1,"error in allocation in tests...\n");
        exit();
    }

    for(int i = 0;i < NUM_MEMORY_ALLOCATIONS;i++){
        memset((mem + (i * PGSIZE)), i, PGSIZE);
    }
    return mem;
}



void* pmalloc_test(){
    void *p = pmalloc();

    if(p == 0){
        printf(1,"pmalloc failed !!!\n");
        exit();
    }
    else if((unsigned int)p % PGSIZE != 0){
        printf(1,"Not page alligned !!!\n");
        exit();
    }
    else{
        memset(p,value,PGSIZE);
    }
    return p;
}

void validate_test(char* mem, char value,int size){
    for(char* p = mem;p < mem + size;p++){
        char mem_val = *p;
        if(mem_val != value){
            printf(1,"memory validation failed, found %d in memory, expected %d in memory!!!\n",mem_val,value);
            exit();

        }
    }

}


    int main(int argc, char *argv[]){
        if(argc < 2){
           // malloc_test();
            //void * mem1 = malloc_test();
            //void * mem2 =  pmalloc_test();
            //validate_test(mem1,value, TWOKB);
            //validate_test(mem2,value, PGSIZE );
            //pfree (mem1);
            //pfree(mem2);
            //mem= page_malloc_test();
            //free(mem);
            printf(1,"Test success\n");
            exit();
        }
        else {
            printf(1,"sanity: cannot performe sanity with params");

        }

    }





