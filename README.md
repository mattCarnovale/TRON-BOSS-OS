<h1 align="center">
 :space_invader: Tron Boss OS :space_invader:<br /> <br />
    <h3 align="center">
      Implementing interesting features to a small unix operating system.
    </h3>
   <p align="center">
    Copyright &copy; 2017 Matt Carnovale
  </p>
</h1>

## About

As an introduction to operating systems, students were tasked with implementing simplified versions of the core concepts covered throughout the course. The project uses [xv6](https://pdos.csail.mit.edu/6.828/2012/xv6.html), a bare bones operating system designed by MIT with the intention to communicate essential concepts and architecture one would find within the Linux and Unix systems.

## Usage

### Prerequisites

- i386-elf-gcc
- i386-elf-gdb (for debugging)
- qumu

\*\* On non-x86 or non-ELF machines a cross-compiler gcc suite capable of producing
x86 ELF binaries must be installed to use "make" commands. Further, the `TOOLPREFIX` flag will have to be adjusted to match that of your environment.

### Installation

Clone the repository to your local machine.<br />
If using HTTPS:<br />
`git clone https://github.com/mattCarnovale/TRON-BOSS-OS.git`<br />
If using SSH:<br />
`git clone https://github.com/mattCarnovale/TRON-BOSS-OS.git`

### Building & Running the Kernel

Build project:<br />
`make` <br />
Remove binaries generated from build:<br />
`make clean` <br />
Default boot mode:<br />
`make qemu-nox` <br />
Debug boot mode: <br />
`make qemu-nox-gdb`

## Deliverables

### Project 1: System Calls

- A system call tracing feature has been added to print the name of each system call
  that is invoked as well as its return value to the console in this format:<br />
  `<system call name> -> <system call return value>`<br />

- The system call facility was implemented using conditional compilation to allow
  this feature to be enabled and disabled using the flag provided in the included `MakeFile`.
- An additional system call, `date()`, has been implemented to return the current
  date and time in Coordinated Universal Time (UTC).

- A new user command, **date** , has been provided to print the current UTC date and
  time returned by the new `date()` system call to the console.

- The value of the `ticks` global variable is now recorded by the process to track
  the time of its creation. The elapsed time for each process can be computed
  using the value documented by this change.

- The functionality of the existing debugging tool accessed through the command
  sequence, `control + p`, has been adapted to include the elapsed time of each
  process in seconds and partial seconds.

### Project 2: Processes

- The process structure has been modified to include a user identifier and a group
  identifier to introduce the concept of ownership to xv6. System calls have been
  added to retrieve the UID, GID, and PPID (parent process identifier); systems calls
  have been added to set the UID and GID. Furthermore, a user command,
  testuidgid , has been implemented to allow the testing of these new attributes
  and routines. The following system calls relate to this feature:

  - **getuid**: retrieve the UID of the current process.
  - **getgid**: retrieve the GID of the current process.
  - **getppid**: retrieve the PPID of the current process if it has one; if it is the
    initial process retrieve the PID.
  - **setuid**: set the UID of the current process to the value passed in.
  - **setgid**: set the GID of the current process to the value passed in.

- The process structure has been modified to include fields that allow the xv6
  system to track the amount of CPU time used by each process.

- A new user command, **ps**, has been provided to report the process identifier,
  name, user identifier, group identifier, parent process identifier, elapsed time,
  CPU time, the process state, and the size of each active process in the system.
  The following system call relates to this feature:

  - **getprocs**: Retrieves information regarding active processes from the
    process table into the table of uproc structures up to a maximum of 1, 16,
    64, or 72.

- An additional user command, **time**, has been implemented to measure the
  number of seconds it takes for a program to be executed and report it in this
  format: <br />
  `<program name> ran in <calculated number> seconds`

- The functionality of the existing debugging tool accessed through the command
  sequence, `control + p`, has been adapted to include the new process provided by
  the first two tasks of this project. In addition to its existing output, `control + p`
  now reports the UID, GID, PPID, CPU time, state, and size of each process.

### Project 3: Improved Process Management

- New lists have been added to the process structure to upgrade the efficiency of
  the process management mechanisms used by the xv6 system. Processes are now organized
  by state in the following lists:

  - ready
  - free
  - sleep
  - zombie
  - running
  - Embryo <br/>

  _These lists are now managed and maintained by a series of helper functions that
  have been implemented in project 3._

- A key invariant has been introduced to the system: a process will be on one list,
  and only one, list at a time. Every period of transition is locked and treated as an
  atomic set of instructions to guarantee that this invariant holds as each
  processes passes from state to state. A process in the **UNUSED** state must
  pass through the **EMBRYO** state to continue. From the **EMBRYO** state, the only
  option for a process is to enter the **RUNNABLE** state or fail and return to the
  **UNUSED** state. If in the **RUNNABLE** state, a process must then enter the
  **RUNNING** state. The **RUNNING** state is one of the few crossroads in the system.
  It can either be put to sleep, entering the **SLEEP** state; it can yield and return to
  the **RUNNABLE** state; it can also exit to the **ZOMBIE** state--never more than one.
  Finally, once a process enters the **ZOMBIE** state it has one option. It must return
  to the **UNUSED** state.

- Round-Robin scheduling has been introduced to the system. Processes that enter
  the ready list are scheduled to run before those that have been added after it.

- Four console control sequences have been implemented to act as a debugging
  tool and reference to ensure that processes are respecting the invariant
  introduced to the system. They are as follows:

  - Control-F: reports the number of processes in the free list at the time of
    invocation. This data is reported in the format: <br />
    `FREE LIST SIZE: <number> processes.`
  - Control-R: reports the number of processes in the ready list at the time of
    invocation. This information is reported in the format: <br />
    `READY LIST PROCESSES: <pid> → <pid> → <pid>`
  - Control-S: reports the number of processes in the sleep list at the time of
    invocation. This information is reported in the format: <br />
    `SLEEP LIST PROCESSES: <pid> → <pid> → <pid>`
  - Control-Z: reports the number of processes in the zombie list at the time
    of invocation. This information is reported in the format: <br />
    `ZOMBIE LIST PROCESSES: <pid, ppid> → <pid, ppid>`

- Four forms of testing have been added and made available to the system.
  One is to be used as a debugging aid during development and can be accessed by enabling the `CFLAGS += -DDEBUG` flag in the Makefile. It is used to determine whether a process has been lost. The other three are available as shell commands: <br />
  - **pstest** : a modified version of forktest that can be used to test the ps command as well as verify the accuracy of the control-f and control-s console commands. It takes an integer as an argument and forks the number of processes that match the number. Furthermore, it can be used
    to validate that the free list and sleep lists are functioning correctly.
  - **Loopforever** : a modified version of forktest that will create 8 child processes off the original process and have them loop from the running list to the ready list. It can be used to verify the correctness of the control-r command and provide evidence that round-robin scheduling is occurring.
  - **Undead** : will fork a total of 5 child processes off the original and they will immediately exit. The parent process is put to sleep for a period of time before calling the wait() routine, so the child processes can be reaped. This provides the means to test the control-z command. Additionally, it can be used to ensure the zombie list is being managed correctly.

### Project 4: MLFQ Scheduler

### Project 5: File System Protection
