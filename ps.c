//This file ps.c includes the process status (ps) commmand which can be
//used to retrieve information regarding active processes (RUNNABLE, 
//SLEEPING, RUNNING, or ZOMBIE);
#include "types.h"
#include "user.h"
#include "uproc.h"


int
main(int argc, char *argv[])
{

  struct uproc * table;
  int max = 16;
  int active_processes;
 
  table = malloc(max * sizeof(struct uproc));

  if(!table) {
    printf(2, "Error: malloc call failed. %s at line %d\n", __FILE__, __LINE__);
    exit();
  }

  //This print statement is only used to prove the validity of the getprocs routine
  //printf(1,"The max value passed into the getprocs is: %d\n", max);

  active_processes = getprocs(max, table);

  if(active_processes < 0){
    printf(2, "Error: active process count returned irregular value. Failure in"
              "call to getprocs() or copyactiveprocs().\n"); 
  } 


  printf(1, "\tPID \tNAME \tUID \tGID \tPPID \tELAPSED CPU \tSTATE \tSIZE\n");

  for(int i = 0; i < active_processes; ++i){
    //format the elapsed_ticks into seconds and partial seconds for elapsed time
    int seconds = table[i].elapsed_ticks / 100;
    int partial_seconds = table[i].elapsed_ticks % 100;
    //format the CPU_total_ticks into seconds and partial seconds for elapsed time
    int cpu_seconds = table[i].CPU_total_ticks / 100;
    int cpu_partial_seconds = table[i].CPU_total_ticks % 100;
 
    printf(1, "\t%d \t%s \t%d \t%d \t%d \t%d.", table[i].pid, 
              table[i].name, table[i].uid, table[i].gid, table[i].ppid,
              seconds);
    if(partial_seconds < 10) printf(1,"0");
    printf(1,"%d", partial_seconds);

    printf(1,"\t%d.", cpu_seconds);
    if(cpu_partial_seconds < 10) printf(1,"0");
    printf(1,"%d", cpu_partial_seconds);

    printf(1,"\t%s \t%d\n", table[i].state, table[i].size);  
  }
 exit();
}

