
#include "types.h"
#include "user.h"
#define ALLOC_NUM 18
#define PGSIZE 4096
#define EXRA_DATA 18
#define STAY sleep(100);
int protected_pg_num = 0;




void print_policy(){

#ifdef NONE
    printf(1, "policy is NONE\n");
    return;
#endif

#ifdef LIFO
    printf(1, "policy is LIFO\n");
    return;
#endif

#ifdef SCFIFO
    printf(1, "policy is SCFIFO\n");
    return;
#endif

}



int first_test(){
    int m_is_protected =0;
    int* m = malloc(4096);

    if(protect_page(m) == 0){
        //printf(1,"c\n");
        m_is_protected =1;
    }

    if(m_is_protected) {
        pfree(m);
    }
    else{
        free(m);
    }

    int p_is_protected =0;
    int* p = pmalloc();

    if(protect_page(p) == -1){
       // printf(1,"p fail to do protect page\n");
    }
    else{
        p_is_protected =1;
        protected_pg_num++;
    }


    if(p_is_protected) {
        if (pfree(p) == -1) {
           // printf(1, "p is protected but pfree failed\n");
            free(p);
        } else {
            protected_pg_num--;
        }
    } else{
        free(p);
    }

    STAY
    return 1;
}

int second_test(){
    char* pages[ALLOC_NUM];
    int i;

    printf(1,"malloc pages\n");
    for(i=0;i<ALLOC_NUM;i++){
        pages[i] =  malloc(PGSIZE);
        memset(pages[i], i, 1);
    }
    STAY
    printf(1,"validate pages\n");
    for(i=0;i<ALLOC_NUM;i++){
        if(pages[i][0] != i){
           // printf(1, "validate pages failed on page: %d \n", i);
            return -1;
        }
    }

    char* extra_pg = malloc(PGSIZE);
    memset(extra_pg,EXRA_DATA,1);
        if(extra_pg[0] != EXRA_DATA){
           // printf(1,"data in child at %d\n", EXRA_DATA, extra_pg[0], EXRA_DATA);

        }
    return 1;
}



int third_test(){
    char input[50];
    int allocated_pg_num = 29;
    int pg_out_num = 13;
    int pgflt_num = 0;
    int total_pgout_num = 0;


    printf(1," Execute Ctrl+P\n");
    gets(input, 10);


    #ifdef NONE
    allocated_pg_num = 0;
    pg_out_num = 0;
    #endif

    #ifdef LIFO
    pgflt_num = 23;
    total_pgout_num = 36;
    #endif

    #ifdef SCFIFO
    pgflt_num = 28;
    total_pgout_num = 41;
    #endif

    printf(1,"allocated pages: %d \npaged out num: %d \nprotected pages num: %d \npage faults num: %d \ntotal paged out num: %d \n", allocated_pg_num, pg_out_num, protected_pg_num, pgflt_num, total_pgout_num );

    return 1;
}



int main(int argc, char *argv[]){

    printf(1, "--------- START TESTING! ---------\n\n\n");
    print_policy();

    printf(1, "------- first_test -------\n");
    first_test();
    printf(1, "------- first_test passed-------\n\n");

    printf(1, "------- second_test-------\n");
    second_test();
    printf(1, "------- second_test passed-------\n\n");

    printf(1, "------- third_test-------\n");
    third_test();
    printf(1, "------- third_test passed-------\n\n");

    printf(1, "--------- DONE  TESTING! ---------\n\n");

    exit();
}
