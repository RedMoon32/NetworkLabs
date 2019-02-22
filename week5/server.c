#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define SERVER_PORT     2000

test_struct_t test_struct;
result_struct_t res_struct;

#define BUFLEN 1024

void die(char *in){
	perror("socket");
}

void worker(void *buf){
	student* stud = (student*)((char*)buf);
	pid_t tid;
	tid = syscall(SYS_gettid);
	printf("\n\nNew request processing: Thread id : %d\n",tid);
	printf("\nGot new student info:\n Name:%s\n Group:%s\n Age:%u\n",stud->name,stud->group,stud->age);
	printf("Thread sleeping\n");
	sleep(10);
}

void
setup_udp_server_communication() {
	struct sockaddr_in si_me, si_other;
	
	int s, i, slen = sizeof(si_other) , recv_len;
	char buf[BUFLEN];
	
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}
	
	
	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(SERVER_PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
	{
		die("bind");
	}
	while(1)
	{
		printf("\nWaiting for data...");
		fflush(stdout);
		
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
		{
			die("recvfrom()");
		}
		
		pthread_t thread;
		pthread_create(&thread,NULL, worker, (void*)buf);
		pthread_join(thread,NULL);		

		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
		{
			die("sendto()");
		}
	}
	close(s);
}

int
main(int argc, char **argv) {

    setup_udp_server_communication();
    return 0;
}
