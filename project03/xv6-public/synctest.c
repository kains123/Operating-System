#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

#define FILESIZE        (130*512)  
#define BUFSIZE         512
#define BUF_PER_FILE    ((FILESIZE) / (BUFSIZE))

int
main(int argc, char *argv[])
{
    int fd, i; 
    int r, log_val;
    int old_log = -1;
    char data[BUFSIZE];

    printf(1, "syncfiletest starting\n");
    const int sz = sizeof(data);
    for (i = 0; i < sz; i++) {
        data[i] = i % 128;
    }

    printf(1, "1. buffered log check\n");
    fd = open("synctestfile1", O_CREATE | O_RDWR);
    for(i = 0; i < BUF_PER_FILE; i++) {
      if (i % 100 == 0){
        printf(1, "%d bytes written\n", i * BUFSIZE);
      }
      if ((r = write(fd, data, sizeof(data))) != sizeof(data)){
        printf(1, "write returned %d : failed\n", r);
        exit();
      }
      if ((log_val = get_log_val()) < 0) {
        printf(1, "get log val returned %d : failed\n", log_val);
        exit();
      } 
      printf(1, "get_log_val : %d -> %d\n", old_log, log_val);
      old_log = log_val;
    }
    printf(1, "%d bytes written\n", BUF_PER_FILE * BUFSIZE);
    close(fd);

    printf(1, "2. sync check\n");
    fd = open("synctestfile2", O_CREATE | O_RDWR);
    for(i = 0; i < BUF_PER_FILE; i++) {
      if (i % 100 == 0){
        printf(1, "%d bytes written\n", i * BUFSIZE);
      }
      if ((r = write(fd, data, sizeof(data))) != sizeof(data)){
        printf(1, "write returned %d : failed\n", r);
        exit();
      }
      if ((old_log = get_log_val()) < 0) {
        printf(1, "get log num returned %d : failed\n", log_val);
        exit();
      } 
      if (sync()) {
        printf(1, "sync failed\n");
        exit();
      }

      if ((log_val = get_log_val()) < 0) {
        printf(1, "get log num returned %d : failed\n", log_val);
        exit();
      } 
      printf(1, "get log num : %d -> %d\n", old_log, log_val);

    }
    printf(1, "%d bytes written\n", BUF_PER_FILE * BUFSIZE);
    close(fd);

    exit();
}

