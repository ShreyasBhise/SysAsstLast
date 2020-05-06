#include<unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<strings.h>
#include<signal.h>
int parentpid;
int childpid;
void exitHandle(int sig) {
	printf("Signal received, closing all processes. %d\n", sig);	
	kill(parentpid, SIGKILL);
	kill(childpid, SIGKILL);
}
//Run TEST code
int main(int argc, char** argv){
	signal(SIGINT, exitHandle);
	int pid = fork();
	if(pid == 0) {//Child Process -> Should be server.
		chdir("./Server");
		system("./WTFServer localhost 6782");
		return 0;
	} else { //Parent Process -> Should handle client.
		childpid = pid;
		parentpid = getpid();
		int fd = open(".Commands", O_RDONLY);
		char* cmd = (char*)malloc(256);
		char c;
		int n;
		int i = 0;
		chdir("./Client");
		while((n = read(fd, &c, 1))>0){
			if(c=='\n'){
				cmd[i]='\0';
				char cmd2[300];
				sprintf(cmd2, "cd Client; %s", cmd);
				system(cmd2);
				sleep(3);
				i = 0;
				bzero(cmd, 255);
			} else {
				cmd[i] = c;
				i++;
			}
		}
		system("diff output.txt expectedOutput.txt");
		close(fd);
		free(cmd);
		return 0;
	}
	return 0;
}
