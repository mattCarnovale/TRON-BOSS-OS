#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "uproc.h"


struct StateLists {
  struct proc * ready;
  struct proc * free;
  struct proc * sleep;
  struct proc * zombie;
  struct proc * running;
  struct proc * embryo;
};

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  struct StateLists pLists;
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

//STATE MANAGEMENT FUNCTIONS 
static void addtofrontoflist(struct proc ** sLists, struct proc * p);
static int removefromfront(struct proc ** sLists, struct proc *p);
static void assertstate(struct proc * p, enum procstate state);
static int removefromlist(struct proc ** sLists, struct proc * p);
static int addtoreadylist(struct proc *p);
static int checkforchildren(struct proc ** sLists, struct proc * parent);
static void adoptzombiechildren(struct proc ** sLists, struct proc * parent);
#ifdef DEBUG
static void checkProcs(char * s);
#endif
  
void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

#ifndef CS333_P3P4
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

#else 
  acquire(&ptable.lock);
  p = ptable.pLists.free;
  if(p){
/*    p = ptable.pLists.free;
    ptable.pLists.free = ptable.pLists.free -> next;
    p->next = 0;
*/
    int rc = removefromfront(&ptable.pLists.free, p);
    if(rc < 0){
      panic("Failed to remove from embryo.");
    }    
    assertstate(p, UNUSED);   
    goto found;
   
  }
  release(&ptable.lock);
  return 0;
#endif

found:
  p->state = EMBRYO;
  addtofrontoflist(&ptable.pLists.embryo, p); 
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    acquire(&ptable.lock);
    int rc = removefromlist(&ptable.pLists.embryo, p);
    if(rc < 0){
      panic("Failure removing from embryo list in allocproc.");
    }
    assertstate(p, EMBRYO);    
    p->state = UNUSED;
    addtofrontoflist(&ptable.pLists.free, p);
    release(&ptable.lock);
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  p -> start_ticks = ticks;
  p -> cpu_ticks_total = 0;
  p -> cpu_ticks_in = 0;
  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  acquire(&ptable.lock);
  //Initialize SLEEP, ZOMBIE, RUNNING, EMBRYO lists explicitly
  ptable.pLists.embryo  = 0;
  ptable.pLists.ready   = 0;
  ptable.pLists.sleep   = 0;
  ptable.pLists.zombie  = 0;
  ptable.pLists.running = 0;
  ptable.pLists.free    = 0;

   //Initialize the FREE List
//#ifdef CS333_P3P4
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	addtofrontoflist(&ptable.pLists.free, p);   //Insertion helper function
  }
//#endif 
  release(&ptable.lock);
  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  //Set the uid field to the default for the initial process
  initproc -> uid = DEFAULT_UID;
  //Set the gid field to the default for the initial process
  initproc -> gid = DEFAULT_GID;

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  acquire(&ptable.lock);
  int rc = removefromlist(&ptable.pLists.embryo, p );
  if(rc < 0){
    panic("Failure to remove from embryo list in userinit");
  } 
  assertstate(p, EMBRYO);
  p->state = RUNNABLE; 
  p->next = 0;
  ptable.pLists.ready = p;
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    acquire(&ptable.lock);
    int rc = removefromlist(&ptable.pLists.embryo, np );
    if(rc < 0){
      panic("Failure to remove from embryo list in userinit");
    } 
    assertstate(np, EMBRYO);
    np->state = UNUSED;
    addtofrontoflist(&ptable.pLists.free, np);
    release(&ptable.lock);
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
  //copy the uid of the parent process to the child
  np -> uid = proc -> uid;
  //copy the gid of the parent process to the child
  np -> gid = proc -> gid;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));
 
  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  int rc = removefromlist(&ptable.pLists.embryo, np);
  if(rc < 0){
    panic("Failure to remove from embryo list in userinit");
  } 
  assertstate(np, EMBRYO);
  np->state = RUNNABLE;
  addtoreadylist(np); 
  release(&ptable.lock);
  
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifndef CS333_P3P4
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
#else
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  p = ptable.pLists.embryo;
  if(p){
    adoptzombiechildren(&ptable.pLists.embryo, proc); 
  } 
  p = ptable.pLists.ready;
  if(p){
    adoptzombiechildren(&ptable.pLists.ready, proc); 
  } 
  p = ptable.pLists.sleep;
  if(p){
    adoptzombiechildren(&ptable.pLists.sleep, proc);
  } 
  p = ptable.pLists.running;
  if(p){
    adoptzombiechildren(&ptable.pLists.running, proc); 
  }

  //Our special case
  p = ptable.pLists.zombie;
  while(p){
     if(p->parent == proc){
       p -> parent = initproc;
       wakeup1(initproc);
     }
     p = p -> next;      
  }
  int rc = removefromlist(&ptable.pLists.running, proc);  
  if(rc < 0 ){
     panic("Failure to remove from running list in exit");
  }
  assertstate(proc, RUNNING);
  proc -> state = ZOMBIE;
  addtofrontoflist(&ptable.pLists.zombie, proc); 
  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}
#endif

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
#ifndef CS333_P3P4
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else
int
wait(void)
{
  struct proc *p;
  int pid;
  int havekids;

  acquire(&ptable.lock);
  for(;;) {
    havekids = 0;

   if((checkforchildren(&ptable.pLists.embryo, proc) == 0)  ||
      (checkforchildren(&ptable.pLists.ready, proc) == 0)   ||
      (checkforchildren(&ptable.pLists.running, proc) == 0) ||
      (checkforchildren(&ptable.pLists.sleep, proc) == 0))  {
      havekids = 1;
      }

    p = ptable.pLists.zombie;
    while(p){
      if(p -> parent == proc){ 
        havekids = 1; 
        int rc = removefromlist(&ptable.pLists.zombie, p);
        if(rc < 0){
          panic("Failure to remove from zombie list in wait");
        }
        assertstate(p, ZOMBIE);
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        addtofrontoflist(&ptable.pLists.free, p);
        release(&ptable.lock);
        return pid;
      }
      p = p -> next;
    }

    if((havekids == 0) || (proc -> killed )){
       release(&ptable.lock);
       return -1;
    }
    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#endif

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifndef CS333_P3P4
// original xv/6 scheduler. Use if CS333_P3P4 NOT defined.
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      //set the time in cpu value to start when the process enters a cpu
      p -> cpu_ticks_in = ticks;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
     sti();
      hlt();
    }
  }
}

#else
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    p = ptable.pLists.ready;
    if(p){
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);
      int rc = removefromfront(&ptable.pLists.ready, p);
      if(rc < 0){
        panic("Failed to remove from ready list in scheduler.");
      }    
      assertstate(p, RUNNABLE);
      p->state = RUNNING;
      addtofrontoflist(&ptable.pLists.running, p);
      //set the time in cpu value to start when the process enters a cpu
      p -> cpu_ticks_in = ticks;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
     sti();
      hlt();
    }
  }
}
#endif

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
#ifndef CS333_P3P4
void
sched(void)
{
  int intena;

  //update the time in cpu value when the process is removed 
  proc -> cpu_ticks_total += ticks - proc -> cpu_ticks_in; 

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}
#else
void
sched(void)
{
  int intena;

  //update the time in cpu value when the process is removed 
  proc -> cpu_ticks_total += ticks - proc -> cpu_ticks_in; 

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}
#endif

// Give up the CPU for one scheduling round.
void
yield(void)
{
#ifndef CS333_P3P4
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
#else
  acquire(&ptable.lock);  //DOC: yieldlock
  removefromlist(&ptable.pLists.running, proc);
  assertstate(proc, RUNNING);
  proc->state = RUNNABLE;
  int rc = addtoreadylist(proc);
  if(rc < 0){
    panic("Failure to add to ready list in yield");
  }
  sched();
  release(&ptable.lock);
#endif
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot 
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// 2016/12/28: ticklock removed from xv6. sleep() changed to
// accept a NULL lock to accommodate.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){
    acquire(&ptable.lock);
    if (lk) release(lk);
  }
#ifndef CS333_P3P4
  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

#else
  // Go to sleep.
  proc->chan = chan;
  int rc = removefromlist(&ptable.pLists.running, proc);
  if(rc < 0){
    panic("Failed to remove from running list in sleep ");
  }
  assertstate(proc, RUNNING);
  proc->state = SLEEPING;
  addtofrontoflist(&ptable.pLists.sleep, proc);
  sched();

  // Tidy up.
  proc->chan = 0;

#endif
  // Reacquire original lock.
  if(lk != &ptable.lock){ 
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}

//PAGEBREAK!
#ifndef CS333_P3P4
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}
#else
static void
wakeup1(void *chan)
{
  struct proc *p = ptable.pLists.sleep;

  struct proc *temp;

  while(p){
     temp = p -> next;  
     if(p -> chan == chan){
        if(removefromlist(&ptable.pLists.sleep, p) == 0){
          assertstate(p, SLEEPING);
          p -> state = RUNNABLE;
          int rc = addtoreadylist(p);
          if(rc < 0){
            panic("Failed to add to ready list in wakeup1.");
          }
        }
      }
     p = temp; 
 }
  return;
}
#endif

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
#ifndef CS333_P3P4
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#else
int
kill(int pid)
{
  acquire(&ptable.lock);
  struct proc *p;

  p = ptable.pLists.sleep;
  while(p){
    if(p->pid == pid){
       int rc = removefromlist(&ptable.pLists.sleep, p);
       if(rc < 0){
         panic("failure to remove from sleep list in kill. ");
       }
       assertstate(p, SLEEPING);
       p->state = RUNNABLE;
       p->killed = 1;
       addtoreadylist(p);
       release(&ptable.lock);
       return 0;
    }
    p = p->next;
  }
  p = ptable.pLists.embryo;
  while(p){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    p = p->next; 
  }
  p = ptable.pLists.ready;
  while(p){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    p = p->next;
  }
  p = ptable.pLists.running;
  while(p){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    p = p->next;
  }

  release(&ptable.lock);
  return -1;
}
#endif

static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
};

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  uint ppid;
  
  cprintf("\nPID \tNAME \tUID \tGID \tPPID \tELAPSED CPU \tSTATE \tSIZE \tPCs\n");

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

    int end_tick = ticks - p->start_ticks;
    int seconds = end_tick/100;
    int partial_seconds = end_tick % 100;
    int cpu_seconds = p -> cpu_ticks_total / 100;
    int cpu_partial_seconds = p -> cpu_ticks_total % 100;
 
   
    if (p-> parent) {
      ppid = p -> parent -> pid;
    } else {
      ppid = p -> pid;
    }
    cprintf("%d \t%s \t%d \t%d \t%d \t%d.", p -> pid, 
              p -> name, p -> uid, p -> gid, ppid,
              seconds);
    if(partial_seconds < 10) cprintf("0");
    cprintf("%d", partial_seconds);

    cprintf("\t%d.", cpu_seconds);
    if(cpu_partial_seconds < 10) cprintf("0");
    cprintf("%d", cpu_partial_seconds);

    cprintf("\t%s \t%d", state, p -> sz);  

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
  cprintf("\n");
}

int
copyactiveprocs(uint max, struct uproc * utable)
{
  struct proc * p; 
  int active_processes = 0;

  acquire(&ptable.lock);  

  for(p = ptable.proc; p < &ptable.proc[NPROC] && active_processes < max; p++){
    if(p->state == UNUSED || p -> state == EMBRYO)
      continue; 
    //Copy the date from any RUNNABLE, SLEEPING, RUNNING, or ZOMBIE processes
    utable -> pid  = p -> pid; 
    utable -> uid  = p -> uid; 
    utable -> gid  = p -> gid;
    if (p-> parent){
      utable -> ppid = p -> parent -> pid;
    } else {
      utable -> ppid = p -> pid;
    }
    utable -> size = p -> sz;
    utable -> elapsed_ticks = ticks - p->start_ticks;    
    utable -> CPU_total_ticks = p -> cpu_ticks_total;
    strncpy(utable -> name, p -> name, sizeof(p -> name)+ 1);
    strncpy(utable -> state, states[p -> state], sizeof(p -> state) + 1);
    ++utable;
    ++active_processes;	
  }
  release(&ptable.lock);  
  return active_processes;
}

//GENERAL STATE LIST ROUTINES
static void 
addtofrontoflist(struct proc ** sLists, struct proc * p){

  if (!p) {return;}
  
  p -> next = *sLists;
  *sLists = p;
  return;
}

static void
assertstate(struct proc * p, enum procstate state) {

  if(p->state == state) {return;}

  panic("ERROR: unexpected process state. ");

}

static int 
removefromfront(struct proc ** sLists, struct proc *p){
   
  if(!*sLists) {return -1;}   

  p = *sLists;
  *sLists = p -> next;
  p -> next = 0;
  return 0;    
}

static int 
removefromlist(struct proc ** sLists, struct proc * p){
  
  if((!p) || !(*sLists)) {return -1;}  
 
  struct proc * current = *sLists;
  struct proc * previous; 

  if(current == p){
    *sLists  = current -> next;
    p -> next = 0;
    return 0;
  }
  current = current -> next;
  previous = *sLists;
  while(current){
    if(current == p){
      previous -> next = p -> next;
      p -> next = 0;
      return 0;
    } else {
        previous = current;
        current = current -> next;
    } 
  }
  return -1;
}

static int 
addtoreadylist(struct proc *p){

  if(!p) {return -1;}

  //Case 2: ready list is empty
  if(!ptable.pLists.ready) {
    p -> next = 0;
    ptable.pLists.ready = p;
    return 0;
  }

  struct proc * current = ptable.pLists.ready;  //for traversal

  while(current -> next) {
    current = current -> next;
  }
  current -> next = p;
  p -> next = 0;
  return 0;
}

static int
checkforchildren(struct proc ** sLists, struct proc * parent) {

  if(!*sLists) {return -1;}

  //transport back in time to CS 162 and set up pointers
  //for traversal
  struct proc * current = *sLists; // set to next in list

  while(current) {
    if(current -> parent == parent){
      return 0;
    } else {
        current = current -> next;
    }
  }

  return -1;    //A match could not be located.
}

static void
adoptzombiechildren(struct proc ** sLists, struct proc * parent) {

  if(!*sLists) {return;}

  struct proc * current = *sLists; // set to next in list

  while(current) {
    if(current -> parent == parent){
      current -> parent = initproc;
    } 
    current = current -> next;    
  }

  return;    
}

//NEW CONTROL SEQUENCES
void
cfreelist(void){

  acquire(&ptable.lock);
  int count = 0;
  struct proc * current = ptable.pLists.free;

  while(current) {
    ++count;
    current = current -> next;
  }
  cprintf("FREE LIST SIZE: %d processes.\n", count);
  release(&ptable.lock);
  return;
}

void
creadylist(void){

  acquire(&ptable.lock);

  if(!ptable.pLists.ready){
    cprintf("READY LIST EMPTY\n");
    release(&ptable.lock);
    return; 
  }

  struct proc * current = ptable.pLists.ready;

  cprintf("READY LIST PROCESSES: ");
  while(current -> next){
    cprintf("%d -> ", current -> pid);
    current = current -> next;
    if(!current -> next){
      cprintf("%d \n", current -> pid);
      release(&ptable.lock);
      return;   
    }
  }
  release(&ptable.lock);
  return;
}

void
csleeplist(void){

  acquire(&ptable.lock);
  if(!ptable.pLists.sleep){
    cprintf("SLEEP LIST EMPTY\n");
    release(&ptable.lock);
    return; 
  }

  struct proc * current = ptable.pLists.sleep;

  cprintf("SLEEP LIST PROCESSES: ");
  while(current -> next){
    cprintf("%d -> ", current -> pid);
    current = current -> next;
    if(!current -> next){
      cprintf("%d \n", current -> pid);
      release(&ptable.lock);
      return;   
    }
  }
  release(&ptable.lock);
  return;
}

void
czombielist(void){

  acquire(&ptable.lock);
  if(!ptable.pLists.zombie){
    cprintf("ZOMBIE LIST EMPTY\n");
    release(&ptable.lock);
    return; 
  }

  struct proc * current = ptable.pLists.zombie;

  cprintf("ZOMBIE LIST PROCESSES: ");
  while(current -> next){
    cprintf("%d, PPID: %d  -> ", current -> pid, current -> parent -> pid);
    current = current -> next;
    if(!current -> next){
      cprintf("%d, PPID: %d\n", current -> pid, current -> parent -> pid);
      release(&ptable.lock);
      return;   
    }
  }
  release(&ptable.lock);
  return;
}

//DEBUG P3
#ifdef DEBUG
static int
findProc(struct proc *p)
{
  struct proc *np;
 
  np = ptable.pLists.free;
  while (np != 0) {
    if (np == p) return 1;
    np = np->next;
  }
  np = ptable.pLists.embryo;
  while (np != 0) {
    if (np == p) return 1;
    np = np->next;
  }
  np = ptable.pLists.running;
  while (np != 0) {
    if (np == p) return 1;
    np = np->next;
  }
  np = ptable.pLists.sleep;
  while (np != 0) {
    if (np == p) return 1;
    np = np->next;
  }
  np = ptable.pLists.zombie;
  while (np != 0) {
    if (np == p) return 1;
    np = np->next;
  }
  np = ptable.pLists.ready;
  while (np != 0) {
    if (np == p) return 1;
    np = np->next;
  }
 
  return 0; // not found
}

// argument 's' is a string to print on not found
static void
checkProcs(char *s)
{
  int found;
  int isholding;
  struct proc *p;

  isholding = holding(&ptable.lock);
  if (!isholding) acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    found = findProc(p);
    if (found) continue;
    cprintf("For checkProcs(%s)\n", s);
    panic("Process array and lists inconsistent\n");
  }
  if (!isholding) release(&ptable.lock);
}
#endif


