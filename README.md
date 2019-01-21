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
- i386-elf-gcc gdb (for debugging)
- qumu

\*\* On non-x86 or non-ELF machines a cross-compiler gcc suite capable of producing
x86 ELF binaries must be installed to use "make" commands. Further, the `TOOLPREFIX` flag will have to be adjusted to match that of your environment.

### Installation

Clone the repository to your local machine with: <br />
If using HTTPS: <br />  
`git clone https://github.com/mattCarnovale/TRON-BOSS-OS.git` <br />
If using SSH: <br />
`git clone https://github.com/mattCarnovale/TRON-BOSS-OS.git`

### Building & Running the Kernel

Build:
`make`

Remove binaries generated from build:
`make clean`

Default boot:
`make qemu-nox`

Debug boot:
`make qemu-nox-gdb`

## Deliverables
