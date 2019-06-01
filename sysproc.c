#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"


int sys_yield(void)
{
  yield(); 
  return 0;
}

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_turn_on_p_flag(void){

  void *va;

  if (argptr(0, (void *) &va, sizeof(va)) < 0){
    return -1;
  }

  return turn_on_p_flag(va);

}


int
sys_turn_off_w_flag(void){

  void *va;

  if (argptr(0, (void *) &va, sizeof(va)) < 0){
    return -1;
  }

  return turn_off_w_flag(va);

}


int
sys_turn_off_p_flag(void){

    void *va;

    if (argptr(0, (void *) &va, sizeof(va)) < 0){
        return -1;
    }

    return turn_off_p_flag(va);

}




int
sys_turn_on_w_flag(void){

  void *va;

  if (argptr(0, (void *) &va, sizeof(va)) < 0){
    return -1;
  }

  return turn_on_w_flag(va);

}



int
sys_turn_on_prsnt_flag(void){

    void *va;

    if (argptr(0, (void *) &va, sizeof(va)) < 0){
        return -1;
    }

    return turn_on_prsnt_flag(va);

}



int
sys_turn_on_user_flag(void){

    void *va;

    if (argptr(0, (void *) &va, sizeof(va)) < 0){
        return -1;
    }

    return turn_on_user_flag(va);

}


int
sys_is_w_flag_off(void){

  void *va;

  if (argptr(0, (void *) &va, sizeof(va)) < 0){
    return -1;
  }

  return is_w_flag_off(va);

}

int
sys_is_p_flag_on(void){

  void *va;

  if (argptr(0, (void *) &va, sizeof(va)) < 0){
    return -1;
  }

  return is_p_flag_on(va);

}



int
sys_inc_protected_pg_num(void){
    inc_protected_pg_num();
    return 1;
}

int
sys_dec_protected_pg_num(void){
    dec_protected_pg_num();
    return 1;
}



//int
//sys_pmalloc(void){
//    pmalloc();
//    return 1;
//
//}
//
//
//int
//sys_protect_page(void){
//    void *va;
//
//    if (argptr(0, (void *) &va, sizeof(va)) < 0){
//        return -1;
//    }
//
//    protect_page(va);
//    return 1;
//
//}
//
//
//
//int
//sys_pfree(void){
//    void *va;
//
//    if (argptr(0, (void *) &va, sizeof(va)) < 0){
//        return -1;
//    }
//
//    pfree(va);
//    return 1;
//
//}