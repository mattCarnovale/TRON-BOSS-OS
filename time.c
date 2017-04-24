//This file time.c includes the time command implementation. This
//command can be used to measure the length of time it takes for
//a program to execute.
#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int num_of_args;
  int process_id;	
  int start_ticks, end_ticks, total_ticks;
  int seconds, partial_seconds;

  //Grab the number of args in addition to the time command	
  num_of_args = argc - 1;

  if(num_of_args < 0) {
    printf(2, "Error: call to time failed\n");
    exit();
  } else if(num_of_args > 0) {
    //Capture current ticks   		  
    start_ticks = uptime();
    
    //Call fork and caputure the process identifier 
    process_id = fork();

    //To manage the parent/child processes follow
    //a format similar to that in chapter 5 of OSTEP
    if(process_id < 0) {		//fork routine failed abnormal exit
      printf(2, "fork failed.\n");    
      exit();
    } else if (process_id == 0) {	//child (new) process 
        ++argv;				//increment to the next argument
	exec(argv[0], argv);		
        printf(2,"Error: Exec routine did not exit normally.\n");	
        exit();
    } else {				//parent process
       process_id = wait();
       end_ticks = uptime();
    }	 
  } else {
     start_ticks = 0;
     end_ticks = 0;
  }

  total_ticks = end_ticks - start_ticks;
  seconds = total_ticks / 100;
  partial_seconds = total_ticks % 100;

  if(num_of_args > 0) { 
    printf(1,"%s ", argv[1]);
  } else { 
    printf(1,"___");     
  }
  printf(1,"ran in: %d.", seconds);
  if(partial_seconds < 10) printf(1,"0");
  printf(1,"%d seconds\n", partial_seconds);

  exit();
}
