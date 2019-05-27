//#include "types.h"
//#include "stat.h"
//#include "user.h"
//#include "fs.h"
//#include "param.h"
//#include "fcntl.h"
//#include "syscall.h"
//#include "traps.h"
//#include "memlayout.h"
//#include "defs.h"
//
//
//#define TWOKB 2048
//#define PGSIZE 4096
//#define NUM_MEMORY_ALLOCATIONS 10
//#define PLUS_TO_PAGE_ALLIGN(p) (PGSIZE - ((uint)p % PGSIZE))
//
//int value = 12345678;
//char *mem;
//
//char * getMemoryAt(int i){
//    return mem + i * PGSIZE;
//}
//
//void* malloc_test(void){
//    void *p = malloc(TWOKB);
//
//    if(p == 0){
//        printf(1,"malloc failed !!!\n");
//        exit();
//    }
//    else{
//        memset(p,value,TWOKB);
//    }
//    return p;
//}
//
//
//void* page_malloc_test(void){
//
//    mem = malloc((NUM_MEMORY_ALLOCATIONS + 1) * PGSIZE);
//
//    mem = mem + PLUS_TO_PAGE_ALLIGN(mem);
//
//    if((uint)mem % PGSIZE != 0){
//        printf(1,"error in allocation in tests...\n");
//        exit();
//    }
//
//    for(int i = 0;i < NUM_MEMORY_ALLOCATIONS;i++){
//        memset(getMemoryAt(i), i, PGSIZE);
//    }
//}
//
//
//
//void* pmalloc_test(){
//    void *p = pmalloc();
//
//    if(p == 0){
//        printf(1,"pmalloc failed !!!\n");
//        exit();
//    }
//    else if((unsigned int)p % PGSIZE != 0){
//        printf(1,"Not page alligned !!!\n");
//        exit();
//    }
//    else{
//        memset(p,value,PGSIZE);
//    }
//    return p;
//}
//
//int validate_test (void* p, int value){
//    char mem_val = *p;
//    if (mem_val != value){
//        printf(1, "falied validate memory");
//        return -1;
//    }
//    return 0;
//
//
//}
//
//
//    int main(int argc, char *argv[]){
//        if(argc < 2){
//            void * mem1 = malloc_test();
//            void * mem2 =  pmalloc_test();
//            if (validate_test(mem1,value)== -1) {
//                goto bad;
//            }
//            if (validate_test(mem1,value)== -1) {
//                goto bad;
//            }
//            pfree (mem1);
//            pfree(mem2);
//            mem= page_malloc_test();
//            free(mem);
//            printf(1,"Test success\n");
//            exit();
//        }
//        else {
//            panic("sanity: cannot performe sanity with params");
//
//        }
//
//    bad:
//        printf(1, "sanity falied");
//    }
//
//
//
//
//
