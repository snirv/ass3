#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  //backup and init values
  uint swaped_pages_num_bkp = curproc->swaped_pages_num;
  uint pysc_pages_num_bkp = curproc->pysc_pages_num;
  int creation_counter = curproc->creation_counter;
  curproc->creation_counter = 0;
  curproc->pysc_pages_num = 0;
  curproc->swaped_pages_num=0;

    struct swap_page swaped_pages_arr_bkp[MAX_TOTAL_PAGES - MAX_PSYC_PAGES];

    for( i = 0 ; i< (MAX_TOTAL_PAGES - MAX_PSYC_PAGES); i++){
        swaped_pages_arr_bkp[i] = curproc->swaped_pages_arr[i];
    }

    struct pysc_page pysc_pages_arr_bkp[MAX_PSYC_PAGES];

  for( i = 0 ; i< (MAX_PSYC_PAGES); i++){
      pysc_pages_arr_bkp[i] = curproc->pysc_page_arr[i];

  }
    memset(curproc->pysc_page_arr,0,MAX_PSYC_PAGES* sizeof(struct pysc_page));
    memset(curproc->swaped_pages_arr,0,(MAX_TOTAL_PAGES-MAX_PSYC_PAGES)* sizeof(struct swap_page));


  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;

  //remove and create new swap file 2.3
  removeSwapFile(curproc);
  createSwapFile(curproc);

  switchuvm(curproc);
  freevm(oldpgdir);


  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }

  // using backup to restore
  curproc->pysc_pages_num = pysc_pages_num_bkp;
  curproc->swaped_pages_num = swaped_pages_num_bkp;
  curproc->creation_counter = creation_counter;


    for( i = 0 ; i< (MAX_TOTAL_PAGES - MAX_PSYC_PAGES); i++){
        curproc->swaped_pages_arr[i] = swaped_pages_arr_bkp[i] ;
    }

    for( i = 0 ; i< (MAX_PSYC_PAGES); i++){
        curproc->pysc_page_arr[i] = pysc_pages_arr_bkp[i];

    }

  return -1;
}
