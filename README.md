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
  sequence, control-p, has been adapted to include the elapsed time of each
  process in seconds and partial seconds.

### Project 2: Processes

### Project 3: Improved Process Management

### Project 4: MLFQ Scheduler

### Project 5: File System Protection
