// zombietest will fork many child processes.
// each child will immediately exit
// parent will sleep for a bit before wait so that
// we can witness the zombie state
#include "types.h"
#include "user.h"
#define TPS 100
int
main(int argc, char *argv[])
{
  int i, pid;

  for (i=0; i<5; i++) {
    pid = fork();
    if (pid == 0) {   // child)
      pid = getpid();
      sleep(pid); // try to avoid messed up output from child processes
      int newval = pid;
      printf(1, "Process %d: setting UID and GID to %d\n", pid, newval);
      setuid(newval);
      setgid(newval);
      exit();
    }
  }
  sleep(10 * TPS);  // sleep 10 seconds
  while (wait() != -1)
    wait();

  exit();
}
