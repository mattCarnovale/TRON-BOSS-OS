#include "types.h"
#include "user.h"


int
main(int argc, char *argv[])
{
  char * pathname;
  char * ascii_mode;
  int bad_octal = 0;
  int mode; 
  int rc; 

  if (argc != 3) {
    printf(2,"Error. chown needs 2 arguments: mode  & target\n");
    exit();
  }

  //Found in CS333 Archive, by Mark Morrissey 03.06.2017
  ascii_mode = argv[1];
  if(strlen(ascii_mode) != 4){
    printf(2, "Error. 4 octal digits required.\n");
    exit();
  }

  pathname = argv[2];
  if(strlen(pathname)<= 0){
    printf(2, "Error. Invalid pathname for chmod command\n");
    exit();
  }
  
  // verify ascii_mode in correct range: 0000 - 1777 octal.
  if (!(ascii_mode[0] == '0' || ascii_mode[0] == '1'))
    bad_octal = 1;
  if (!(ascii_mode[1] >= '0' && ascii_mode[1] <= '7'))
    bad_octal = 1;
  if (!(ascii_mode[2] >= '0' && ascii_mode[2] <= '7'))
    bad_octal = 1;
  if (!(ascii_mode[3] >= '0' && ascii_mode[3] <= '7'))
    bad_octal = 1;
  if(bad_octal){
    printf(2, "Error. Octal digit provided out of bounds.\n");
    exit();
  } 

  mode = atoo(ascii_mode);

  rc = chmod(pathname, mode);
  if(rc < 0){
    printf(2, "Error. chown failed to execute.\n");
    exit();
  }

  printf(1, "Mode for %s changed to %s.\n", pathname, ascii_mode);

  exit();
}
