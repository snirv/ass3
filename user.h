struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int yield(void);
void dec_protected_pg_num(void);
void inc_protected_pg_num(void);



//pmalloc
int turn_on_p_flag(void*);  //1.1 turn on protected flag for page  - means that page wa alloced using pmalloc


//protect_page
int is_p_flag_on(void*);  //1.1
int turn_off_p_flag(void*);


int turn_off_w_flag (void*);



//pfree
int turn_on_w_flag(void*);  //1.1 turn on write flag for page
int is_w_flag_off(void*);  //1.1

// ulib.c
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
unsigned int strlen(char*);
void* memset(void*, int, unsigned int);
void* malloc(unsigned int);
void free(void*);
int atoi(const char*);
int protect_page(void* ap);
void* pmalloc(void);
int pfree(void*);
