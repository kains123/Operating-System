#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if(argc != 3){
    printf(2, "Usage: ln old new\n");
    exit();
  }
  if(link(argv[1], argv[2]) < 0) {
    printf(2, "link %s %s: failed\n", argv[1], argv[2]);
  } else {
    if (symlink(argv[2], argv[3]) < 0)
      printf(2, "symlink %s %s: failed\n", argv[2], argv[3]);
  }

  exit();
}
