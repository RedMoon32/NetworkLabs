#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <errno.h> 
#include <sys/wait.h>
#include "stack.h"


int fds[2];
int pid;
int child_status;

char commands[7][30] = {"Push","Pop","Peek","IsEmpty","Display","Create","Stack_Size"};
int commands_with_input[7]= {1,0,0,0,0,0,0};

void server(){
	char buf[256];
	int reading_input = 0;
	while (1){
		close(fds[1]);
		read(fds[0],buf,32);
		write(fds[0],"-x",32);
		printf("Buffer is %s\n",buf);
		if (strcmp(buf,"Push")==0){
			read(fds[0],buf,32);
			kill(pid,SIGCONT);
			while (strcmp(buf,"-x")==0){
				read(fds[0],buf,32);
			}
			sscanf(buf,"%d",reading_input);
			push(reading_input);
			printf("Push %d command",reading_input);
		}
		else if (strcmp(buf,"Pop")==0){
			printf("Pop command \n");
			pop();
		}
		else if (strcmp(buf,"Peek")==0){
			printf("Peek command: %d \n",peek());
		}
		else if (strcmp(buf,"IsEmpty")==0){
			printf("Is empty: %d \n",empty());
		}
		else if (strcmp(buf,"Display")==0){
			printf("Display command: \n");
			display();
		}
		else if (strcmp(buf,"Create")==0){
			create();
			printf("Create stack ");
		}
		else if (strcmp(buf,"Stack size")==0){
			printf("Stack size command ");
			stack_size();
		}
		else{
			printf("No new command");
		}
	}
}

void client(){

	char buf[256];
	int ncommands = 7;

	printf("Available commands:\n");
	for (int i=0;i<ncommands;i++){
		printf("%s ",commands[i]);
	}
	printf("\n");
	close(fds[0]);
	while (1){
		printf("=============\nCommand is: ");
		scanf("%s",buf);
		int found = 0;

		if (strcmp(buf,"exit") == 0){
			printf("Exiting");
			return;
		}

		for (int i=0;i<ncommands;i++){
			int cmp = strcmp(buf,commands[i]);
			if (cmp==0){
				found = 1;
				write(fds[1],buf,32);
				if (commands_with_input[i]){
					printf("Input is:");
					scanf("%s",buf);
					write(fds[1],buf,32);
				}
				break;
			}
		}
		if (!found){
			printf("Command not found, please try again:\n");
		}
	}
}

int main(){
	pipe(fds);
	pid = fork();
	if (pid>0) {
		server();
	}
	else if (pid==0){
		client();
	}
	else{
		printf("ERROR");
	}
}