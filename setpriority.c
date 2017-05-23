//This file includes the implementation for the setpriority command.
//The setpriority command can be used to change the priority a process
//has in the ready list MLFQ
#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int num_of_args;
  int rc;
  int pid;
  int priority;


  num_of_args = argc - 1; 

  if(num_of_args < 2 || num_of_args > 2){
    printf(2, "Error: takes two integer arguments\n");
    exit();
  }

  pid = atoi(argv[1]);
  priority = atoi(argv[2]);
  
  rc = setpriority(pid, priority);
  if(rc == -1){
    printf(2, "Error: system call routine unsuccessful.\n");
    exit();
  } else if(rc == -2){
      printf(2, "Error: invalid pid request.\n");
      exit();
  } else if(rc == -3){
      printf(2, "Error: invalid priority request.\n");
      exit();
  } else if(rc == -4){
      printf(2, "Error: process not found in state lists.\n");
      exit();
  } else if (rc == -5){
      printf(1, "No change: priority is already %d.\n", priority);
      exit();
  } 

  printf(1, "Set process %d to priority level: %d.\n", pid, priority);
  exit();
}
