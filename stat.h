#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

struct stat {
  short type;  // Type of file
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short nlink; // Number of links to file
#ifdef CS333_P5
  ushort uid;           // Owner ID
  ushort gid;           // Group ID
//  union stat_mode_t mode; //protection/mode bits 
#endif
  uint size;   // Size of file in bytes
};
