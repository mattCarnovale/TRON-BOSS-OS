//This file, testuidgid.c contains the implementation for an example
//test for part of project 2 of CS_333. Specifically, the getters 
//and setters for the user identifier and group identifier,
//as well as the parent process identifier
#include "types.h"
#include "user.h" 
#define TPS 100

static void
uidtest (uint nval)
{
  uint uid = getuid();
  printf(1, "Current UID is: %d\n", uid);
  printf(1, "Setting UID is: %d\n", nval);
  if (setuid(nval) < 0)
    printf(2, "Error Invalid UID: %d\n", nval);
  setuid(nval);
  uid = getuid();
  printf(1, "Current UID is: %d\n", uid);
  sleep(5 * TPS);	//now type control-p
}
  
  
static void
gidtest (uint nval)
{
  uint gid = getgid();
  printf(1, "Current GID is: %d\n", gid);
  printf(1, "Setting GID is: %d\n", nval);
  if (setgid(nval) < 0)
    printf(2, "Error Invalid GID: %d\n", nval);
  setgid(nval);
  gid = getgid();
  printf(1, "Current GID is: %d\n", gid);
  sleep(5 * TPS);	//now type control-p
}
  
static void
forktest (uint nval)
{
  uint uid, gid;
  int pid;
  
  printf(1, "Setting UID to %d and GID to %d before fork().  Value"
		   " should be inherited\n", nval, nval);

  if (setuid(nval) < 0)
    printf(2, "Error Invalid UID: %d\n", nval);
  if (setgid(nval) < 0)
    printf(2, "Error Invalid GID: %d\n", nval);

  printf(1, "Before fork(), UID is %d, GID is %d\n", getuid(), getgid());
  pid = fork();
  if (pid == 0)		//child
  {
    uid = getuid();
    gid = getgid();
    printf(1, "Child UID is: %d, GID is: %d\n", uid, gid); 
    sleep(5 * TPS);	//now type control-p
    exit();
  }
  else
  sleep(10 * TPS);	//wait for child to exit before proceeding
}

static void
invalidtest (uint nval)
{
  printf(1, "Setting UID to %d. This test should FAIL\n", nval);
  if (setuid(nval) < 0)
    printf(1, "SUCCESS! The setuid system call indicated failure\n");
  else
    printf(1, "FAIL! The setuid system call indicated success\n");
    
  printf(1, "Setting GID to %d. This test should FAIL\n", nval);
  if (setgid(nval) < 0)
    printf(1, "SUCCESS! The setgid system call indicated failure\n");
  else
    printf(1, "FAIL! The setgid system call indicated success\n");
    
  printf(1, "Setting UID to %d. This test should FAIL\n", -1);
  if (setuid(-1) < 0)
    printf(1, "SUCCESS! The setuid system call indicated failure\n");
  else
    printf(1, "FAIL! The setuid system call indicated success\n");
    
  printf(1, "Setting GID to %d. This test should FAIL\n", -1);
  if (setgid(-1) < 0)
    printf(1, "SUCCESS! The setgid system call indicated failure\n");
  else
    printf(1, "FAIL! The setgid system call indicated success\n");
}    

static int
testuidgid(void)
{
  uint nval, ppid;

  // get/set uid test
  nval = 100;
  uidtest(nval);

  // get/set gid test
  nval = 200;
  gidtest(nval);

  // get ppid test
  ppid = getppid();
  printf(1, "My parent process is: %d\n", ppid);

  // fork tests to demonstrate UID/GID inheritance
  nval = 111;
  forktest(nval);

  // tests for invalid values for uid and gid
  nval = 32800;		//32767 is max value
  invalidtest(nval);

  printf(1, "Done!\n");
  return 0;
}

int
main()
{
  testuidgid();
  exit();
}
