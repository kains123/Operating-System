#include "param.h"
#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "user.h"



static void testsym(void);
static void cleanup(void);

int
main(int argc, char *argv[])
{
  cleanup();
  testsym();
  exit();
}

static void
cleanup(void)
{
  unlink("/testsym/a");
  unlink("/testsym/b");
  unlink("/testsym/c");
  unlink("/testsym/1");
  unlink("/testsym/2");
  unlink("/testsym/3");
  unlink("/testsym/4");
  unlink("/testsym/z");
  unlink("/testsym/y");
  unlink("/testsym");
}


// stat a symbolic link using O_NOFOLLOW
static int
stat_slink(char *pn, struct stat *st)
{
  int fd = open(pn, O_RDONLY | O_NOFOLLOW);
  if(fd < 0)
    return -1;
  if(fstat(fd, st) != 0)
    return -1;
  return 0;
}

static void
testsym(void)
{
  int r, fd1 = -1, fd2 = -1;
  char buf[4] = {'a', 'b', 'c', 'd'};
  char buf1[4] = {'a', 'a', 'a', 'a'};
  char c = 0;
  struct stat st;
    
  printf(1, "Start: test symlinks\n");

  mkdir("/testsym");

  fd1 = open("/testsym/a", O_CREATE | O_RDWR);
  if(fd1 < 0) printf(1,"failed to open a");

  r = symlink("/testsym/a", "/testsym/b");
  if(r < 0)
    printf(1,"symlink b -> a failed");

  if(write(fd1, buf, sizeof(buf)) != 4)
    printf(1,"failed to write to a");

  if (stat_slink("/testsym/b", &st) != 0)
    printf(1,"failed to stat b");
  if(st.type != T_SYMLINK)
    printf(1,"b isn't a symlink");

  fd2 = open("/testsym/b", O_RDWR);
  if(fd2 < 0)
    printf(1,"failed to open b");
  if(write(fd1, buf1, sizeof(buf)) != 4)
    printf(1,"failed to write to b");
  read(fd2, &c, 1);
  if (c != 'a')
    printf(1,"failed to read bytes from b");
  
}
