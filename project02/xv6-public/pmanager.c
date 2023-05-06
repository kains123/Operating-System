#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int stacksize = 100;
  char *cmd1 = "l"; //list
	char *cmd2 = "k"; //kill
	char *cmd3 = "e"; //execute
	char *cmd4 = "m"; //memlim
	char *cmd5 = "x"; //exit

	for(;;){
		char array[30];
    gets(array,30);
    array[strlen(array)-1] = 0;

    if(array[0]==cmd1[0]){
			printf(1,"LIST\n");
			list();
			continue;
		}
    if(array[0]==cmd2[0]){
      printf(1,"KILL\n");
      char *pid = (char*)malloc(sizeof(char)*15);
      int j = 0;
			for(int i=5;i<strlen(array);i++){
				pid[j] = array[i];
				j++;
			}
      pid[j] = '\0';
			int pid1 = atoi(pid);
			kill(pid1);
			continue;
		}
    if(array[0]==cmd3[0]){
      printf(1,"EXECUTE\n");
			char* stacksize = (char*)malloc(sizeof(char)*10000000);	//1million
			continue;
		}
    if(array[0]==cmd4[0]){
      printf(1,"MEMLIM\n");
      int pid = 0;
      int limit1 = 100;
      setmemorylimit(pid,limit1);
			continue;
		}
    if(array[0]==cmd5[0]){
      printf(1,"EXIT\n");
      exit();
		}
  }
  //list
  //kill
  //execute
  //memlim
	// exit();
};
