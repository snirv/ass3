#define MAX_TOTAL_PAGES 32
#define MAX_PSYC_PAGES 16



// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };



struct pysc_page
{
    pte_t* pte;            // PTE that mapped to the pa
    int creation_time;    // counts ticks passed from creation time
};


struct swap_page
{
    pte_t* pte;            // PTE that mapped to the pa
};



// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  int creation_counter;        // global counter per proccess

    //Swap file. must initiate with create swap file
    struct file *swapFile;      //page file
    //PCB meta-data
    uint pysc_pages_num;          // number of physic pages the process holds added 2.1
    uint swaped_pages_num;         // number of swaped pages the process holds added 2.1

    //swaped_pages_array - an array to hold the pte's of the pages in the swapFile 2.1
    struct swap_page swaped_pages_arr[MAX_TOTAL_PAGES - MAX_PSYC_PAGES];

    struct pysc_page  pysc_page_arr[MAX_PSYC_PAGES];  // will store data on all pages (RAM & DISK)d
    int protectes_pg_num;
    int pgflt_num;
    int page_out_num;
};



// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
