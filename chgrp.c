#include "types.h"
#include "user.h"


int
main(int argc, char *argv[])
{
  int group;
  char * pathname;  
  int rc; 

  if (argc != 3) {
    printf(2,"Error. chgrp needs 2 arguments: group & target\n");
    exit();
  }

  pathname = argv[2];
  if(strlen(pathname)<= 0){
    printf(2, "Error. Invalid pathname for chgrp command\n");
    exit();
  }

  group = atoi(argv[1]);
  if (group < 0 || group > 32767){
    printf(2, "Error. Invalid GID for chgrp command--out of bounds.\n");
    exit();
  } 

  rc = chgrp(pathname, group);   
  if(rc < 0){
    printf(2, "Error. chgrp failed to execute.\n");
    exit();
  }

  printf(1, "The GID %d has been applied to: %s.\n", group, pathname);

  exit();
}
