#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "uproc.h"

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
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
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
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      return -1;
    }
    sleep(&ticks, (struct spinlock *)0);
  }
  return 0;
}

// return how many clock tick interrupts have occurred
// since start. 
int
sys_uptime(void)
{
  uint xticks;
  
  xticks = ticks;
  return xticks;
}

//Turn of the computer
int sys_halt(void){
  cprintf("Shutting down ...\n");
// outw (0xB004, 0x0 | 0x2000);  // changed in newest version of QEMU
  outw( 0x604, 0x0 | 0x2000);
  return 0;
}

//implementation of the date() system call
int
sys_date(void)
{
  struct rtcdate *d;    

  if(argptr(0, (void*)&d, sizeof(*d)) < 0)
    return -1;
  
  cmostime(d);
  return 0;
}

//implementation of the getuid() system call
uint
sys_getuid(void)
{
  return proc->uid;
}

//implementation of the getgid() system call
uint
sys_getgid(void)
{
  return proc->gid;
}

//implementation of the getppid() system call
uint
sys_getppid(void)
{
  if(proc -> parent)	//Check if the parent flag is null
    return proc -> parent -> pid;	
  
  return proc->pid;
}

//implementation of the setuid() system call
int
sys_setuid(void)
{
  int new_uid;	

  //This statement uses the argint routine implemented in syscall.c
  //to grab the nth 32-bit system call argument as an integer.
  argint(0, &new_uid);	  
  if((new_uid < 0) || (new_uid > 32767))
    return -1;

  proc -> uid = new_uid;	     
  return 0;	
}

//implementation of the setgid() system call
int
sys_setgid(void)
{
  int new_gid;	

  argint(0, &new_gid);
  if((new_gid < 0) || (new_gid > 32767))
    return -1;

  proc -> gid = new_gid;	     
  return 0;	
}

//implementation of the getprocs system call
int
sys_getprocs(void)
{
  int array_size;
  struct uproc * t;

  if(argint(0, &array_size) < 0 ||  
     argptr(1, (void*)&t, sizeof(*t)) < 0)  //Similar to the way it's used in sys_date 
    return -1;      

  return copyactiveprocs(array_size, t);
}

int
sys_setpriority(void)
{
  int pid, priority;

  if(argint(0, &pid) < 0 || argint(1, &priority) < 0)
    return -1;

  return setpriority(pid, priority);
}
