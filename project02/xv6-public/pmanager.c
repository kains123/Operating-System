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
  char *cmd1 = "list"; //list
	char *cmd2 = "kill"; //kill <pid>
	char *cmd3 = "execute"; //execute <path> <stacksize>
	char *cmd4 = "memlim"; //memlim <pid> <limit>
	char *cmd5 = "exit"; //exit

	printf(1,"[PMANAGER]\n\n");

	while(getcmd(array, sizeof(array)) >= 0){

		while((fd = open("console", 0x002)) >= 0) {
			if(fd >= 3) {
				close(fd);
				break;
			}
		}

		if(array[0]==cmd5[0] && array[1]==cmd5[1] && array[2]==cmd5[2] && array[3]==cmd5[3]){
			printf(1,"\n<<EXIT>>\n");
			exit();
			continue;
		} else if(array[0]==cmd1[0]){
			printf(1,"\n<<LIST>>\n");
			list();
			continue;
		} else if(array[0]==cmd2[0]){
			//kill <pid>
			printf(1,"\n<<KILL>>\n");
			char *pid = (char*)malloc(sizeof(char)*15);
			int j = 0;

			
			for(int i=5; i<strlen(array); i++){
				pid[j] = array[i];
				j++;
			}

			pid[j] = '\0';
			int pid_num = atoi(pid);

			printf(1, "------ %d ------\n", pid_num);
			if(kill(pid_num) == -1) {
				printf(1, "kill failed\n");
			} else {
				printf(1, "kill success\n\n");
			}
			printf(1, "*********WHAT'S WRONG 0*******\n\n");
			continue;
		} else if(array[0]==cmd3[0]){
			printf(1,"<<EXECUTE>>\n");
			int pid;
			int stacksize = 100;
			int index = 8;
			char path[1024];
			pid = fork();
			int path_index = 0;

			while(array[index] != ' ' && array[index] != '\n') {
				if(index > 40) {
					array[index] = '\n';
					break;
				}				
				path[path_index] = array[index];
				path_index++;
				index++;
				path[path_index] = '\0';
			}

			argv[0] = path;
			argv[1] = 0;

			while(48 <= array[index] && array[index] <= 57) {
				stacksize *= 10;
				stacksize += array[index] - 48;
				index++;
			}

			pid = fork();

			if(pid == 0) {
				pid = fork();
				if(pid == 0) {
					exec2(argv[0], argv, stacksize);
					printf(1, "failed to exec\n");
				} 
				else if(pid < 0) {
					printf(1, "fork failed\n");
				}
				exit();

			} 
			else if(pid > 0) {
				wait();
			} 
			else {
				printf(1, "failed to fork\n");
			}
			continue;
	  } else if(array[0]==cmd4[0]){
			printf(1,"\n<<MEMLIM>>\n");
			int pid = 0;
			int limit = 100;
			int idx = 7;
			// if(array[idx] == " " || array[idx] == "\n"){
			// 	continue;
			// }

			while (48 <= array[idx] && array[idx] <= 57)
			{
				pid *= 10;
				pid += array[idx] - 48;
				idx++;
			}
			idx++;
			
			while(48 <= array[idx] && array[idx] <= 57) {
				limit *= 10;
				limit += array[idx] - 48;
				idx++;
			}

			if(setmemorylimit(pid, limit) == -1) {
				printf(1, "setmemorylimit failed\n");
			} else {
				printf(1, "set memory limit success\n\n");
			}	
			continue;
	  }
  }
	exit();
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


