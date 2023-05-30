#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if(argc != 3  && (argc != 4 || (argc == 4 && !strcmp(argv[2], "-s")))){
    printf(2, "Usage: ln old new or symlink\n");
    exit();
  }
  printf(2, "%s********\n",argv[1]);
  if(argv[1] == "-h") {
    if(link(argv[2], argv[3]) < 0) {
      printf(2, "link %s %s: failed\n", argv[1], argv[2]);
    }
  }
  if (argv[1] == "-s") {
    if (symlink(argv[2], argv[3]) < 0) {
      printf(2, "symlink %s %s: failed\n", argv[2], argv[3]);
    }
  }

  exit();
}
