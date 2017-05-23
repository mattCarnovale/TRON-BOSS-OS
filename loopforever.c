#include "types.h"
#include "user.h"
#define TPS 100

int
main(void)
{
  int pid, max = 7;
  unsigned long x = 0;

  for (int i=0; i<max; i++) {
    sleep(5*TPS);  // pause before each child starts
    pid  = fork();
    if (pid < 0) {
      printf(2, "fork failed!\n");
      exit();
    }

    if (pid == 0) { // child
      sleep(getpid()*100); // stagger start
      do {
	x += 1;
      } while (1);
      printf(1, "Child %d exiting\n", getpid());
      exit();
    }
  }

  pid = fork();
  if (pid == 0) {
    sleep(15);
    do {
      x = x+1;
    } while (1);
  }

  sleep(10*TPS);
  wait();
  printf(1, "Parent exiting\n");
  exit();
}
