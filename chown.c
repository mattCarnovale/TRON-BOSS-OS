#include "types.h"
#include "user.h"


int
main(int argc, char *argv[])
{
  int owner;
  char * pathname;  
  int rc; 

  if (argc != 3) {
    printf(2,"Error. chown needs 2 arguments: pathname & user id\n");
    exit();
  }

  pathname = argv[2];
  if(strlen(pathname)<= 0){
    printf(2, "Error. Invalid pathname for chown command\n");
    exit();
  }

  owner = atoi(argv[1]);
  if (owner < 0 || owner > 32767){
    printf(2, "Error. Invalid UID for chown command--out of bounds.\n");
    exit();
  } 

  rc = chown(pathname, owner);   
  if(rc < 0){
    printf(2, "Error. chown failed to execute.\n");
    exit();
  }

  printf(1, "The UID %d has been applied to: %s.\n", owner, pathname);

  exit();
}
