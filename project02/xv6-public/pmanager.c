#include "types.h"
#include "stat.h"
#include "user.h"

int getcmd(char *buf, int nbuf);
char *argv[10];
int fd;

int
main(int argc, char *argv[])
{
	char array[1024];
  char *cmd1 = "l"; //list
	char *cmd2 = "k"; //kill
	char *cmd3 = "e"; //execute
	char *cmd4 = "m"; //memlim
	char *cmd5 = "x"; //exit
	printf(1,"[PMANAGER]\n");
	// while(getcmd(array, sizeof(array)) >= 0){
  //   printf(1,"[PMANAGER]\n");
	// 	while((fd = open("console", 0x002)) >= 0) {
	// 			if(fd >= 3) {
	// 				close(fd);
	// 				break;
	// 			}
	// 		}

	// 		if(array[0]==cmd1[0]){
	// 			printf(1,"LIST\n");
	// 			list();
	// 			continue;
	// 		}
	// 		if(array[0]==cmd2[0]){
	// 			printf(1,"KILL\n");
	// 			char *pid = (char*)malloc(sizeof(char)*15);
	// 			int j = 0;
	// 			for(int i=5;i<strlen(array);i++){
	// 				pid[j] = array[i];
	// 				j++;
	// 			}
	// 			pid[j] = '\0';
	// 			int pid1 = atoi(pid);
	// 			kill(pid1);
	// 			continue;
	// 		}
	// 		if(array[0]==cmd3[0]){
	// 			printf(1,"EXECUTE\n");
	// 			char* stacksize = (char*)malloc(sizeof(char)*10000000);	//1million
	// 			continue;
	// 		}
	// 		if(array[0]==cmd4[0]){
	// 			printf(1,"MEMLIM\n");
	// 			int pid = 0;
	// 			int limit = 100;
	// 			if(setmemorylimit(pid, limit) == -1) {
	// 				printf(1, "setmemorylimit failed.\n");
	// 			}
	// 			continue;
	// 		}
	// 		if(array[0]==cmd5[0]){
	// 			printf(1,"EXIT\n");
	// 			exit();
	// 		}
  // }
};


int
getcmd(char *buf, int nbuf)
{
	printf(2, "> ");
	memset(buf, 0, nbuf);
	gets(buf, nbuf);
	
	if(buf[0] == 0)
		return -1;
	return 0;
}


