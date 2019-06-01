#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(turn_on_p_flag)
SYSCALL(turn_off_w_flag)
SYSCALL(turn_on_w_flag)
SYSCALL(is_w_flag_off)
SYSCALL(is_p_flag_on)
SYSCALL(dec_protected_pg_num)
SYSCALL(inc_protected_pg_num)
SYSCALL(turn_off_p_flag)
SYSCALL(turn_on_prsnt_flag)
SYSCALL(turn_on_user_flag)
//SYSCALL(pmalloc)
//SYSCALL(protect_page)
//SYSCALL(pfree)


