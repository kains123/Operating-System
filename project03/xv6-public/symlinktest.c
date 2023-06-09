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
  char c = 0;
   c2 = 0;
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
  read(fd2, &c, 1);
  if (c != 'a')
    printf(1,"failed to read bytes from b");

  unlink("/testsym/a");

  if(open("/testsym/b", O_RDWR) >= 0)
    printf(1,"Should not be able to open b after deleting a");

  r = symlink("/testsym/b", "/testsym/a");
  if(r < 0)
    printf(1,"symlink a -> b failed");

  r = open("/testsym/b", O_RDWR);
  if(r >= 0)
    printf(1,"Should not be able to open b (cycle b->a->b->..)\n");
  
  r = symlink("/testsym/nonexistent", "/testsym/c");
  if(r != 0)
    printf(1,"Symlinking to nonexistent file should succeed\n");

  r = symlink("/testsym/2", "/testsym/1");
  if(r) printf(1,"Failed to link 1->2");
  r = symlink("/testsym/3", "/testsym/2");
  if(r) printf(1,"Failed to link 2->3");
  r = symlink("/testsym/4", "/testsym/3");
  if(r) printf(1,"Failed to link 3->4");

  close(fd1);
  close(fd2);

  fd1 = open("/testsym/4", O_CREATE | O_RDWR);
  if(fd1<0) printf(1,"Failed to create 4\n");
  fd2 = open("/testsym/1", O_RDWR);
  if(fd2<0) printf(1,"Failed to open 1\n");

  c = '#';
  r = write(fd2, &c, 1);
  if(r!=1) printf(1,"Failed to write to 1\n");
  r = read(fd1, &c2, 1);
  if(r!=1) printf(1,"Failed to read from 4\n");
  if(c!=c2)
    printf(1,"Value read from 4 differed from value written to 1\n");
  printf(1, "test symlinks: ok\n");

  close(fd1);
  close(fd2);
}
