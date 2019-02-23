#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include "common.h"

#define PORT            2000
#define SERVER "192.168.2.22"

student client_data;
result_struct_t result;

void die(char *s){
	perror(s);
}

#define BUFLEN 1024
void setup_tcp_communication() {
    struct sockaddr_in si_other;
	int s, i, slen=sizeof(si_other);
	char buf[BUFLEN];
	char message[BUFLEN];

	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	
	if (inet_aton(SERVER , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	student stud;
	while(1)
	{
		printf("New student:\n");
		printf("Enter name : ");
		scanf("%s",stud.name);
		printf("Enter age : ");
		scanf("%u",&stud.age);
		printf("Enter group : ");
		scanf("%s",stud.group);		

		//send the message
		if (sendto(s, &stud, sizeof(stud) , 0 , (struct sockaddr *) &si_other, slen)==-1)
		{
			die("sendto()");
		}
	}

	close(s);
}
    

int main(int argc, char **argv) {
    setup_tcp_communication();
    printf("application quits\n");
    return 0;
}

