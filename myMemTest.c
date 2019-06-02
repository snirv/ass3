//#include "types.h"
//#include "stat.h"
//#include "user.h"
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
//        memset((mem + (i * PGSIZE)), i, PGSIZE);
//    }
//    return mem;
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
//void validate_test(char* mem, char value,int size){
//    for(char* p = mem;p < mem + size;p++){
//        char mem_val = *p;
//        if(mem_val != value){
//            printf(1,"memory validation failed, found %d in memory, expected %d in memory!!!\n",mem_val,value);
//            exit();
//
//        }
//    }
//
//}
//
//
//    int main(int argc, char *argv[]){
//        if(argc < 2){
//           // malloc_test();
//            //void * mem1 = malloc_test();
//            //void * mem2 =  pmalloc_test();
//            //validate_test(mem1,value, TWOKB);
//            //validate_test(mem2,value, PGSIZE );
//            //pfree (mem1);
//            //pfree(mem2);
//            //mem= page_malloc_test();
//            //free(mem);
//            printf(1,"Test success\n");
//            exit();
//        }
//        else {
//            printf(1,"sanity: cannot performe sanity with params");
//
//        }
//
//    }
//
//
//
//
//



// Created by Dor Green on 14/05/2019.
//





//TODO change the test a bit


#include "user.h"
#include "types.h"
#include "mmu.h"
#include "user.h"

//
//int test_no = 0;
//
//int very_simple(int pid){
//    if(pid == 0){
//        //printf(1, "rest...\n");
//        sleep(100);
//        //printf(1, "exit...\n");
//        exit();
//    }
//    if(pid > 0){
//        printf(1, "Parent waiting on test %d\n", test_no);
//        wait();
//        printf(1, "Done!\n");
//        return 1;
//    }
//    return 0;
//}
//
//
//int simple(int pid){
//    if(pid == 0){
//        printf(1, "try alloc, access, free...\n");
//        char* trash = malloc(3);
//        trash[1] = 't' ;
//        if(trash[1] != 't'){
//            printf(1,"Wrong data in simple! %c\n", trash[1]);
//        }
//        free(trash);
//        printf(1, "Done malloc dealloc, exiting...\n");
//        exit();
//    }
//    if(pid > 0){
//        printf(1, "Parent waiting on test %d\n", test_no);
//        wait();
//        printf(1, "Done! malloc dealloc\n");
//        return 1;
//    }
//    return 0;
//}
//
//
//int test_paging(int pid, int pages){
//    if(pid == 0){
//        int size = pages;
//        char* pp[size];
//        for(int i = 0 ; i < size ; i++){
//            printf(1, "Call %d for sbrk\n", i);
//            pp[i] = sbrk(PGSIZE-1);
//            *pp[i] = '0' +(char) i ;
//        }
//        printf(1, "try accessing this data...\n");
//
//        if(size > 4 && *pp[4] != '4'){
//            printf(1,"Wrong data in pp[4]! %c\n", *pp[4]);
//        }
//
//        if(*pp[0] != '0'){
//            printf(1,"Wrong data in pp[0]! %c\n", *pp[0]);
//        }
//
//        if(*pp[size-1] != '0' +(char) size-1 ){
//            printf(1,"Wrong data in pp[size-1]! %c\n", *pp[size-1]);
//        }
//
//        printf(1, "Done allocing, exiting...\n");
//        exit();
//
//    }
//
//    if(pid >  0){
//        printf(1, "Parent waiting on test %d\n", test_no);
//        wait();
//        printf(1, "Done! alloce'd some pages\n");
//        return 1;
//    }
//    return 0;
//}
//






int task1(){
    int* m = malloc(4096);
    int* p = pmalloc();

    if(protect_page(m) == 1){
        printf(1,"nununu- it protected page that was allocated with malloc ! \n");
        free(m);
        pfree(p);
        return 0;
    }

    if(protect_page(p) == 0){
        printf(1,"nununu- protected page failed ! \n");
        free(m);
        pfree(p);
        return 0;

    }
    free(m);
    if(pfree(p) == -1){
        printf(1, "pfree failed!\n");
        return 0;
    }

    int *p2 = pmalloc();
    protect_page(p2);

    return 1;
}




int main(int argc, char *argv[]){

    printf(1, "--------- START TESTING! ---------\n");


    printf(1, "------- test%d -------\n", 1);
    task1();

    printf(1, "------- test%d -------\n", 2);
   // simple(fork());
    //test_no++;

    printf(1, "------- test%d -------\n", 3);
//    test_paging(fork(),2);
//    test_no++;

    printf(1, "------- test%d -------\n", 4);
//    test_paging(fork(),6);
//    test_no++;

    printf(1, "------- test%d -------\n", 5);
//    test_paging(fork(),13);
//    test_no++;



    printf(1, "--------- DONE  TESTING! ---------\n");
    return 0;
}
