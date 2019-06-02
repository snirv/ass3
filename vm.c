#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"


int
get_page_index_in_swap_arr(pte_t *pte);
void insert_to_pysc_arr(pte_t* pte , int index_to_insert);
int get_index_in_pysc_arr(pte_t* pte);
pte_t* get_page_to_page_out(struct proc* p);
int find_page_min_creation(struct proc* p);

extern char data[];  // defined by kernel.ld
pde_t *kpgdir;  // for use in scheduler()

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;

  a = (char*)PGROUNDDOWN((uint)va);
  last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
  for(;;){
    if((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if(*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
//
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNBASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap {
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};

// Set up kernel part of a page table.
pde_t*
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
  if (P2V(PHYSTOP) > (void*)DEVSPACE)
    panic("PHYSTOP too high");
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                (uint)k->phys_start, k->perm) < 0) {
      freevm(pgdir);
      return 0;
    }
  return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void
kvmalloc(void)
{
  kpgdir = setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void
switchkvm(void)
{
  lcr3(V2P(kpgdir));   // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void
switchuvm(struct proc *p)
{
  if(p == 0)
    panic("switchuvm: no process");
  if(p->kstack == 0)
    panic("switchuvm: no kstack");
  if(p->pgdir == 0)
    panic("switchuvm: no pgdir");

  pushcli();
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
                                sizeof(mycpu()->ts)-1, 0);
  mycpu()->gdt[SEG_TSS].s = 0;
  mycpu()->ts.ss0 = SEG_KDATA << 3;
  mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
  // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
  // forbids I/O instructions (e.g., inb and outb) from user space
  mycpu()->ts.iomb = (ushort) 0xFFFF;
  ltr(SEG_TSS << 3);
  lcr3(V2P(p->pgdir));  // switch to process's address space
  popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(pde_t *pgdir, char *init, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
  memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int
loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
  uint i, pa, n;
  pte_t *pte;

  if((uint) addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pte);
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, P2V(pa), offset+i, n) != n)
      return -1;
  }
  return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
    struct proc* p = myproc();
  char *mem;
  uint a;
  if(newsz >= KERNBASE)
    return 0;
  if(newsz < oldsz)
    return oldsz;


//todo check if need to block init and sh + check pgdir


  a = PGROUNDUP(oldsz);
  for(; a < newsz; a += PGSIZE){
      #if SELECTION != NONE

          if (p->pysc_pages_num + p->swaped_pages_num == MAX_TOTAL_PAGES) { // proc cannot pass 32 pages total 2.1
//              cprintf("pass max total paging allocuvm proc name:%s\n",p->name);
              return 0;
          }

      #endif

      mem = kalloc();
    if(mem == 0){
      cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }

    memset(mem, 0, PGSIZE);
    if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
      cprintf("allocuvm out of memory (2)\n");
      deallocuvm(pgdir, newsz, oldsz);
      kfree(mem);
      return 0;
    }

#if SELECTION != NONE
      pte_t *pte = walkpgdir(pgdir, (void *) PGROUNDDOWN((uint) a), 0);
      if(pte != 0) {
          if (p->pysc_pages_num == MAX_PSYC_PAGES) { // max pysc pages -- need to swap
                    cprintf("pass max pysc paging allocuvm proc name:%s ,pte:%x\n", p->name, pte);
              int index_of_page_in_pysc_arr = page_out(0,
                                                       0); // 0,0 - page out swap to any index in swap file , in this case swap file must still have place
              insert_to_pysc_arr(pte, index_of_page_in_pysc_arr);

          } else {
                   // cprintf("insert to pysc arr - not max allocuvm proc name:%s , pte:%x\n", p->name, pte);
              int index_to_insert = p->pysc_pages_num;
              insert_to_pysc_arr(pte, index_to_insert);
          }
      }
  }
#endif
//    if(p->pgdir == pgdir && ((namecmp(myproc()->name, "init") != 0) && (namecmp(myproc()->name, "sh") != 0))) {
//        for (int i = 0; i < MAX_PSYC_PAGES; i++) {
//            cprintf("pte[%d] = %x\n", i, myproc()->pysc_page_arr[i].pte);
//        }
//    }
    //cprintf("exit allocuvm proc name:%s\n",p->name);
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz){
  pte_t *pte;
  uint a, pa;

  if(newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
  for(; a  < oldsz; a += PGSIZE){
    pte = walkpgdir(pgdir, (char*)a, 0);
    if(!pte)
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
    else if((*pte & PTE_P) != 0){
      pa = PTE_ADDR(*pte);
      if(pa == 0)
        panic("kfree");
      char *v = P2V(pa);
      kfree(v);
      *pte = 0;
    }
  }
  return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void
freevm(pde_t *pgdir)
{
  uint i;

  if(pgdir == 0)
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  for(i = 0; i < NPDENTRIES; i++){
    if(pgdir[i] & PTE_P){
      char * v = P2V(PTE_ADDR(pgdir[i]));
      kfree(v);
    }
  }
  kfree((char*)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if(pte == 0)
    panic("clearpteu");
  *pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if(!(*pte & PTE_P))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)P2V(pa), PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0)
      goto bad;
  }
  return d;

bad:
  freevm(d);
  return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char*
uva2ka(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if((*pte & PTE_P) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  return (char*)P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int
copyout(pde_t *pgdir, uint va, void *p, uint len)
{
  char *buf, *pa0;
  uint n, va0;

  buf = (char*)p;
  while(len > 0){
    va0 = (uint)PGROUNDDOWN(va);
    pa0 = uva2ka(pgdir, (char*)va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (va - va0);
    if(n > len)
      n = len;
    memmove(pa0 + (va - va0), buf, n);
    len -= n;
    buf += n;
    va = va0 + PGSIZE;
  }
  return 0;
}

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.


//finds used page in page dir and write it to swap file - page out
//return -1 upon failure and the index of the page that was paged out in the pysc arr
int
page_out(int to_swap , int index_to_swap) {
//    cprintf("page out enter proc name:%s\n",myproc()->name);
    uint offset_index;
    struct proc *p = myproc();
    p->page_out_num++; //4.0

    //find a page to page out
    pte_t *pte_to_page_out = get_page_to_page_out(p);
//    cprintf("page out before proc name:%s\n",myproc()->name);
    int index_in_pysc_arr = get_index_in_pysc_arr(pte_to_page_out); // a place for the new page in the pysc arr

//    char * arr = (char *)P2V(PTE_ADDR(*pte_to_page_out));
//    cprintf("arr[0] = %d\n",arr);
    if(to_swap){
        offset_index =  (uint)index_to_swap;

    } else {
        offset_index = p->swaped_pages_num;
    }
//    cprintf("page out before proc name:%s\n",myproc()->name);
    if (writeToSwapFile(p, (char *)P2V(PTE_ADDR(*pte_to_page_out)), offset_index * PGSIZE, PGSIZE) == -1) {
        panic("page out fail on writeToSwapFile\n");
    }
//    cprintf("page out after proc name:%s\n",myproc()->name);
    if (to_swap) {
        p->swaped_pages_arr[index_to_swap].pte = pte_to_page_out;
    } else {
        p->swaped_pages_arr[p->swaped_pages_num].pte = pte_to_page_out;
        p->swaped_pages_num++;
    }


  //set flags
  *pte_to_page_out = *pte_to_page_out | PTE_PG ;
  *pte_to_page_out = *pte_to_page_out & (~PTE_P) ;
    lcr3(V2P(p->pgdir));// refresh the tlb

  //free the page that was paged out
  kfree((char*)P2V(PTE_ADDR(*pte_to_page_out)));
    lcr3(V2P(p->pgdir));// refresh the tlb


    return index_in_pysc_arr;
}


//// get a va that was paged out and load it back to the pysc mem
//// need to swap "rosh be rosh" with page in the pysc mem

/*
 * 1. check if the page was paged out or just sgm fault
 * 2. kalloc mem
 * 3. copy page to mem
 * 4. page out another page from pysc to the index of old one in swap file (remove the old file from pysc arr)
 * 5. map mem to pgdir
 * 6. add a pte to pysc page
 *
 * return -1 upon failure and 1 for success
 */
int
page_in(uint va){

  struct proc* p = myproc();
  char* a =  (char*)PGROUNDDOWN((uint)va);
  pte_t* pte = walkpgdir(p->pgdir,a,0);

    if ((PTE_FLAGS(*pte) & PTE_P) != 0){//check if page is present todo
        cprintf("page is present\n");
        return -1;
    }

  if ((*pte & PTE_PG) == 0){//check if page is not paged out
      cprintf("page was not paged out\n");
        return -1;
    }
//    cprintf("page in before proc name:%s\n",myproc()->name);




    char *mem = kalloc();
  if (mem == 0){
    panic("kalloc faild in page_in");
  }
  memset(mem, 0, PGSIZE);

  //get page from swap file
  int swap_index = get_page_index_in_swap_arr(pte);
    if(swap_index == -1){
        panic("failed to swap in page in\n");
    }
  uint place_on_file = (uint)swap_index * PGSIZE;
  readFromSwapFile(p,mem,place_on_file ,PGSIZE);
//    cprintf("page in after1 proc name:%s\n",myproc()->name);

// if psyc mem is full do page out
  int to_swap = 1;
  int index_to_insert = page_out(to_swap, swap_index);
//    cprintf("page in after proc name:%s\n",myproc()->name);

  //map kalloc to va
  if(mappages(p->pgdir, a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
    kfree(mem);
    panic("mappages faild in page_in");
  }

    //update data strucute
   insert_to_pysc_arr(pte,index_to_insert);
    lcr3(V2P(p->pgdir));// refresh the tlb


    return 1;

}


int get_page_index_in_swap_arr(pte_t *pte){  //sharon added
    struct proc* p = myproc();
    for (int i=0; i< p->swaped_pages_num; i++){
        if (p->swaped_pages_arr[i].pte == pte){
            return i;
        }
    }
    return -1;
}


void insert_to_pysc_arr(pte_t* pte , int index_to_insert){
    if(pte == 0){
        panic("insert_to_pysc_arr got pte = 0\n");
    }
    struct pysc_page page;
    page.pte = pte;
    page.creation_time = myproc()->creation_counter++;
    myproc()->pysc_page_arr[index_to_insert] = page;
    if(myproc()->pysc_pages_num < MAX_PSYC_PAGES){
        myproc()->pysc_pages_num++;
    }

}


int get_index_in_pysc_arr(pte_t* pte){
    struct proc* p = myproc();
    for (int i=0; i < p->pysc_pages_num; i++){
        if(pte == p->pysc_page_arr[i].pte){
            return i;
        }
    }
    return -1;
}

// 1.1 syscalls
int
turn_on_p_flag(void* va){
    pte_t* pte;

    pte = walkpgdir(myproc()->pgdir, va, 0);
    if (pte == 0){
        panic("p-flag pte is 0\n");
    } else{
        *pte = *pte | PTE_PRTC;
        lcr3(V2P(myproc()->pgdir));// refresh the tlb
        return 0;
    }

}

int
turn_off_w_flag(void* va){
    pte_t* pte;

    pte = walkpgdir(myproc()->pgdir, va, 0);
    if (pte == 0){
        panic("w-flag pte is 0\n");
    } else{
        *pte = *pte & (~PTE_W);
        lcr3(V2P(myproc()->pgdir));// refresh the tlb
        return 0;
    }
}


int
turn_off_p_flag(void* va){
    pte_t* pte;

    pte = walkpgdir(myproc()->pgdir, va, 0);
    if (pte == 0){
        panic("p-flag pte is 0\n");
    } else{
        *pte = *pte & (~PTE_PRTC);
        lcr3(V2P(myproc()->pgdir));// refresh the tlb
        return 0;
    }
}


int
turn_on_w_flag(void* va){
    pte_t* pte;

    pte = walkpgdir(myproc()->pgdir, va, 0);
    if (pte == 0){
        panic("w-flag pte is 0\n");
    } else{
        *pte = *pte |PTE_W;
        lcr3(V2P(myproc()->pgdir));// refresh the tlb
        return 0;
    }
}






int
is_w_flag_off(void* va){
    pte_t* pte;

    pte = walkpgdir(myproc()->pgdir, va, 0);
    if (pte == 0){
        panic("w-flag pte is 0\n");
    } else if((*pte & PTE_W) == 0){
        return 1;
    } else{
        return 0;
    }
}


int
is_p_flag_on(void* va){
    pte_t* pte;

    pte = walkpgdir(myproc()->pgdir, va, 0);
    if (pte == 0){
        panic("p-flag pte is 0\n");
    } else if((*pte & PTE_PRTC) != 0){
        return 1;
    } else{
        return 0;
    }
}



int
is_a_flag_on(pte_t* pte){
    if (pte == 0){
       panic("a-flag pte is 0\n");
    } else if((*pte & PTE_A) != 0){
        return 1;
    } else{
        return 0;
    }
}

int
turn_off_a_flag(pte_t* pte){
    if (pte == 0){
        return -1;
    } else{
        *pte = *pte & (~PTE_A);
        lcr3(V2P(myproc()->pgdir));// refresh the tlb
        return 0;
    }
}




//3.0

pte_t* get_page_to_page_out(struct proc* p){

    int index = 0;
#if SELECTION == LIFO
//    cprintf("lifo proc name:%s\n",myproc()->name);
    int max =0;
    for(int i=0; i< p->pysc_pages_num ; i++ ){
            if (p->pysc_page_arr[i].creation_time > max){
                max = p->pysc_page_arr[i].creation_time;
                index = i;
            }
    }
#endif


#if SELECTION == SCFIFO
    cprintf("scfifo proc name:%s\n",myproc()->name);
//    for (int i = 0; i < MAX_PSYC_PAGES; i++) {
//            cprintf("pte[%d] = %x\n", i, p->pysc_page_arr[i].pte);
//        }
    index = find_page_min_creation(p);
    while (is_a_flag_on(p->pysc_page_arr[index].pte)){
        turn_off_a_flag(p->pysc_page_arr[index].pte);
        p->pysc_page_arr[index].creation_time = myproc()->creation_counter++;
        index = find_page_min_creation(p);
        cprintf("scfifo while proc name:%s , pte:%x\n",myproc()->name,p->pysc_page_arr[index].pte);
    }
#endif
    return p->pysc_page_arr[index].pte;
}


int find_page_min_creation(struct proc* p){
    int min_creation_time = p->pysc_page_arr[0].creation_time;
    int index_of_min = 0;
    for(int i=0; i< p->pysc_pages_num ; i++ ){
        if ((p->pysc_page_arr[i].creation_time < min_creation_time )&& (p->pysc_page_arr[i].pte != 0) ){
            min_creation_time = p->pysc_page_arr[i].creation_time;
            index_of_min = i;
        }
    }
//    cprintf("index_of_min %d\n", index_of_min);
    return index_of_min;



}


void dec_protected_pg_num() {
    myproc()->protected_pg_num--;
}

void inc_protected_pg_num(){
    myproc()->protected_pg_num++;
}

