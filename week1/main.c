#include <stdio.h>
#include <string.h>
#include <unistd.h> 
#include <errno.h> 
#include <sys/wait.h>
#include "stack.h"


int fds[2];
int pid;
int child_status;
struct stack* st = NULL;

void server(){
	char buf[256];

	while(1){
		close(fds[1]);
		read(fds[0],buf,32);
		if (strncmp(buf,"push",4)==0){
			int v = 0;
			int i = 5;
			while (buf[i] != '\n' && buf[i]!='\0') {
				printf("%c ",buf[i]);
				v *= 10;
				v += (int)(buf[i] - 48);
				i++;
			}
			push(st,v);
			printf("Pushed %d on stack.\n", v);
		}
		else if (strncmp(buf,"pop",3)==0){
			printf("Pop command\n");
			pop(st);
		}
		else if (strncmp(buf,"create",5)==0){
			st = create();
			printf("Empty stack initialized\n");
		}
		else if (strncmp(buf,"peek",4)==0){
			printf("Peek command:%d\n",peek(st));
		}
		else if (strncmp(buf,"isempty",7)==0){
			printf("Is stack empty:%d\n",empty(st));
		}
		else if (strncmp(buf,"display",7)==0){
			printf("Disply command\n");
			display(st);
		}
		else if (strncmp(buf,"stack_size",10)==0){
			printf("Stack size command\n");
			stack_size(st);
		}
		else{
			printf("Unknown command\n");
		}
		printf("=========\nNew command:\n");
	}
}

void client(){
	char buf[256];
	printf("Available commands: push NUMBER, pop, peek, isempty, display, create, stack_size\nNew command:\n");
	while(1){
		close(fds[0]);
		fgets(buf,32,stdin);
		write(fds[1],buf,32);
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