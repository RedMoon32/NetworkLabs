#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>
#include "shared/list.h"

#define NAME_SIZE 20
#define IP_PORT_SIZE 16
#define NUMBER_OF_FILES 3
#define NUMBER_OF_NODES 10
#define CONN_REQUEST "CONNECT"
#define HELLO_MESSAGE "HELLO"
#define GET_FILE_REQUEST "GET_FILE"
#define HAVE_FILE_REQUEST "HAVE_FILE"
#define SMALL_REQS 20
#define BUFF_SIZE 100

#define OK "OK"
#define ERR "ERROR"
#define PING_REQUEST "PING"



char network_name[NAME_SIZE];
p_array_list nodes;
char files[NUMBER_OF_FILES][NAME_SIZE] = {"1.txt","2.txt","3.txt"};
int ping_fd;
uint16_t current_ping_port;
int main_sock;
typedef struct Node{
    char name[NAME_SIZE];
    char ip[IP_PORT_SIZE];
    char port[IP_PORT_SIZE];
    char ping_port[IP_PORT_SIZE];
    int  ping_count;
} node;

int get_socket(int domain, int type, int protocol){
    int sockfd = socket(domain, type, protocol);
    if (sockfd < 0){ 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
    return sockfd;
}


void* udp_send_ping(void *arg){
    int sping_fd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in servaddr;
    node *current;

    listen(sping_fd,10);
    while (1){
        sleep(2);
        for (int i = array_list_iter(nodes); i != -1; i = array_list_next(nodes, i)) {
            
            current = array_list_get(nodes, i);
            if (strcmp(current->name,network_name)==0)
                continue;
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = AF_INET; 
            uint16_t ping_port;
            sscanf(current->ping_port,"%hu",&ping_port);
            servaddr.sin_port = htons(ping_port); 
            servaddr.sin_addr.s_addr = INADDR_ANY;
            int n, len; 
            char message[100] = {'\0'};
            strcat(message,PING_REQUEST);
            strcat(message," ");
            strcat(message,network_name);
            sendto(sping_fd, message, strlen(message), 
                0, (const struct sockaddr *) &servaddr,  
                    sizeof(servaddr)); 
            current->ping_count--;
            //  printf("Ping has been sent to %s\n",current->name);
            if (current->ping_count<0){
                printf("Node %s - %s:%s does not respond to pings, dropping from list\n",current->name,current->ip,current->ping_port);
                array_list_remove(nodes, current);
                free(current);
            }
        }
    }
}


void* udp_receive_ping(void *arg){
    
    struct sockaddr_in cliaddr;
    memset(&cliaddr, 0, sizeof(cliaddr));
    int len, n; 
    node* current;
    while (1){
        char buffer[SMALL_REQS];
        n = recvfrom(ping_fd, (char *)buffer, SMALL_REQS,  
                    0, ( struct sockaddr *) &cliaddr, 
                    &len); 
        buffer[n]='\0';
        if (strncmp(buffer,PING_REQUEST,strlen(PING_REQUEST)-1)==0){
            
            for (int i = array_list_iter(nodes); i != -1; i = array_list_next(nodes, i)){
                current = array_list_get(nodes, i);
                if (strncmp(current->name,buffer+5,n-5)==0){

                    current->ping_count=5;
                }
            }
        }
    }
}

void tcp_request_network_connection(char* ip,char* port){
    int new_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    uint16_t conn_port;
    sscanf(port,"%hu",&conn_port);
    servaddr.sin_port = htons(conn_port);
    if (connect(new_sock,(struct sockaddr *)&servaddr,sizeof(struct sockaddr))==-1){
        perror("connect");
        exit(1);
    }
    char message[100] = {'\0'};
    strncat(message,"HELLO ",6);
    strncat(message,network_name,strlen(network_name));
    strncat(message," ",1);
    char my_port[IP_PORT_SIZE];
    snprintf(my_port,IP_PORT_SIZE,"%d",get_port(main_sock));
    strncat(message,my_port,strlen(my_port));
    strncat(message," ",1);
    char my_port2[IP_PORT_SIZE];
    snprintf(my_port2,IP_PORT_SIZE,"%d",get_port(ping_fd));
    strncat(message,my_port2,strlen(my_port2));
    printf("Message sent:%s\n",message);
    write(new_sock,message,strlen(message));

    char result[10];
    read(new_sock,result,10);
    printf("Result of connecting:%s\n",result);
    close(new_sock);
}

int get_port(int sock_fd){
    int port;
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(sock_fd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else{
        port = ntohs(sin.sin_port);
    }
    return port;
}


void new_node_info(node* new_node){
    int sock_fd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in servaddr;
    char message[100] = {'\0'};
    strncat(message,"CONNECT ",8);
        
    strncat(message,new_node->name,strlen(new_node->name));
    strncat(message," ",1);

    strncat(message,new_node->ip,strlen(new_node->ip));
    strncat(message," ",1);

    strncat(message,new_node->port,strlen(new_node->port));
    strncat(message," ",1);

    strncat(message,new_node->ping_port,strlen(new_node->ping_port));
    
    for (int i = array_list_iter(nodes); i != -1; i = array_list_next(nodes, i)) {
        node* current = array_list_get(nodes, i);
        if (strcmp(current->name,network_name)==0)
            continue;
        struct sockaddr_in servaddr;
        servaddr.sin_addr.s_addr = inet_addr(current->ip);
        servaddr.sin_family = AF_INET;
        uint16_t conn_port;
        sscanf(current->port,"%hu",&conn_port);
        servaddr.sin_port = htons(conn_port);  
        if (connect(sock_fd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr))==-1){
            perror("connect");
            exit(1);
        }
        write(sock_fd,message,strlen(message));
    }
    close(sock_fd);
}



void send_all_nodes(node* new_node){
    printf("Sending all peer table to new node\n");
    for (int i = array_list_iter(nodes); i != -1; i = array_list_next(nodes, i)) {
        int sock_fd = socket(AF_INET,SOCK_STREAM,0);
        struct sockaddr_in servaddr;
        servaddr.sin_addr.s_addr = inet_addr(new_node->ip);
        servaddr.sin_family = AF_INET;
        uint16_t conn_port;
        sscanf(new_node->port,"%hu",&conn_port);
        servaddr.sin_port = htons(conn_port);  
        if (connect(sock_fd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr))==-1){
            perror("connect");
            exit(1);
        }    
        node* current = array_list_get(nodes, i);
        char message[100] = {'\0'};
        strncat(message,"CONNECT ",8);
        
        strncat(message,current->name,strlen(current->name));
        strncat(message," ",1);

        strncat(message,current->ip,strlen(current->ip));
        strncat(message," ",1);

        strncat(message,current->port,strlen(current->port));
        strncat(message," ",1);

        strncat(message,current->ping_port,strlen(current->ping_port));
        printf("Message sent:%s\n",message);
        write(sock_fd,message,strlen(message));
    }
}

void* downloading(){
    char answer = '0';
        while (answer != 'n') {
            printf("Do you want to download any files? [y/n] ");
            scanf(" %c", &answer);
            if (answer == 'y') {
                char file_name[NAME_SIZE];
                printf("Which file to download from other peers?\n");
                scanf("%s",file_name);
                download_file(file_name);
            }
            getchar();
        }
}

void tcp_listen(){
    
    
    printf("Main socket is listening\n");
    
    
    pthread_t download_thread;
    pthread_create(&download_thread, NULL, downloading, NULL);
    char message[100];
    struct sockaddr_in server_addr, client_addr;
    while (1){
        
        int addr_len = sizeof(client_addr);
        int comm_sock_fd = accept(main_sock, (struct sockaddr *) &client_addr, &addr_len);
        int n = read(comm_sock_fd, message, BUFF_SIZE);
        if (strncmp(HELLO_MESSAGE,message,strlen(HELLO_MESSAGE)-1)==0){
            node* new_connected_node = malloc(sizeof(node));
            memset(new_connected_node,0,sizeof(node)); 
            new_connected_node->ping_count = 5;
            strncpy(new_connected_node->ip,inet_ntoa(client_addr.sin_addr),IP_PORT_SIZE);
            int i = strlen(HELLO_MESSAGE)+1;
            int j = 0;
            //HELLO VM1 192.168.1.15 4343 8282
            while (message[i]!=' '){
                new_connected_node->name[j] = message[i];
                j++;i++;
            }
            j = 0;
            i++;

            while(message[i]!=' '){
                new_connected_node->port[j] = message[i];
                i++;j++;
            }
            j = 0;
            i++;
            while(message[i]!='\0'){
                new_connected_node->ping_port[j] = message[i];
                i++;j++;
            }
            write(comm_sock_fd,"OK",3);
            send_all_nodes(new_connected_node);
            new_node_info(new_connected_node);
            if (not_in_list(new_connected_node->name))
                array_list_add(nodes,new_connected_node);
            printf("New node %s with %s:%s has been connected to the network\n",new_connected_node->name,new_connected_node->ip,new_connected_node->port);
        }
        else if (strncmp(CONN_REQUEST,message,strlen(CONN_REQUEST)-1)==0){
            node* new_connected_node = malloc(sizeof(node));
            memset(new_connected_node,0,sizeof(node)); 
            new_connected_node->ping_count = 5;
            int i = strlen(CONN_REQUEST)+1;
            int j = 0;
            //HELLO VM1 192.168.1.15 4343 8282
            while (message[i]!=' '){
                new_connected_node->name[j] = message[i];
                j++;i++;
            }
            j = 0;
            i++;
            
            while(message[i]!=' '){
                new_connected_node->ip[j] = message[i];
                i++;j++;
            }
            j = 0;
            i++;

            while(message[i]!=' '){
                new_connected_node->port[j] = message[i];
                i++;j++;
            }
            j = 0;
            i++;
            while(message[i]!='\0'){
                new_connected_node->ping_port[j] = message[i];
                i++;j++;
            }
            if (not_in_list(new_connected_node->name))
                array_list_add(nodes,new_connected_node);
            printf("Peers table has been updated with new node %s - %s:%s \n",new_connected_node->name,new_connected_node->ip,new_connected_node->port);
        }
        else if (strncmp(HAVE_FILE_REQUEST,message,strlen(HAVE_FILE_REQUEST)-1)==0){
            message[n]='\0';
            char* file_name = message+strlen(HAVE_FILE_REQUEST)+1;
            printf("Client asks for %s file\n",file_name);
            for (int i=0;i<NUMBER_OF_FILES;i++){
                if (strcmp(files[i],file_name)==0){
                    write(comm_sock_fd,OK,strlen(OK));
                    printf("Found file");
                    break;
                }

            }
           write(comm_sock_fd,ERR,strlen(ERR));
        }
        else if (strncmp(GET_FILE_REQUEST,message,strlen(GET_FILE_REQUEST)-1)==0){
            char* file_name = message+strlen(GET_FILE_REQUEST)-1;
            printf("Sending %s file\n",file_name);
            send_file(file_name,comm_sock_fd);
        }
        close(comm_sock_fd);
    }
}

void download_file(char* name){
    int sock_fd = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in servaddr;
    char message[100]={'\0'};
    
    strcat(message,"HAVE_FILE ");
    strcat(message,name);
    printf("Sending %s ",message);
    for (int i = array_list_iter(nodes); i != -1; i = array_list_next(nodes, i)) {
            node* current = array_list_get(nodes, i);
            if (strcmp(current->name,network_name)==0)
                continue;
            struct sockaddr_in servaddr;
            servaddr.sin_addr.s_addr = inet_addr(current->ip);
            servaddr.sin_family = AF_INET;
            uint16_t conn_port;
            sscanf(current->port,"%hu",&conn_port);
            servaddr.sin_port = htons(conn_port);  
            if (connect(sock_fd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr))==-1){
                perror("connect");
                continue;
            }
            write(sock_fd,message,strlen(message));
            char buffer[100];
            
            read(sock_fd,buffer,100);
            

            if (strncmp(buffer,OK,2)==0){
                char message2[100] = {'\0'};
                printf("File found, getting file\n");
                strcat(message2,GET_FILE_REQUEST);
                strcat(message2," ");
                strcat(message2,name);
                connect(sock_fd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr));
                write(sock_fd,message2,100);
                printf("Sending %s\n",message2);
                int n = read(sock_fd,buffer,100); 
                   
                buffer[n] = '\0';
                int number_of_words;
                sscanf(buffer,"%d",&number_of_words);
                printf("Getting %s file of %d words",name,number_of_words);
                for (int i=0;i<number_of_words;i++){
                    n = read(sock_fd,buffer,100);
                    printf("Got new word:",buffer);
                }
            }    
    }
    close(sock_fd);   
}

void send_file(char* file,int comm_sock_fd){
    char source[1000000];
    char symbol;
    FILE *fp = fopen("1.txt", "r");
    if(fp != NULL)
    {
        while((symbol = getc(fp)) != EOF)
        {
            strcat(source, &symbol);
        }
        fclose(fp);
    }
    int count_of_words = 0;
    for (int i=0;i<strlen(source);i++){
        if (source[i] == ' ')
            count_of_words++;
    }
    char strcount[10];
    snprintf(strcount,10,"%d",count_of_words);

    write(comm_sock_fd,strcount,strlen(strcount));
    char *word = strtok(source," ");
    while (word!=NULL){
        write(comm_sock_fd,word,strlen(word));
        word = strtok(NULL," ");
    }
}

int not_in_list(char* name){
    for (int i = array_list_iter(nodes); i != -1; i = array_list_next(nodes, i)) {
        node* current = array_list_get(nodes, i);
        if (strcmp(current->name,name)==0)
            return 0;
    }
    return 1;
}

int main() {
    main_sock = get_socket(AF_INET, SOCK_STREAM, 0);
    listen(main_sock,10);
    ping_fd = get_socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(0); 
      
    // Bind the socket with the server address 
    if ( bind(ping_fd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    printf("Main socket port:%d\n",get_port(main_sock));
    printf("Ping port:%u\n",get_port(ping_fd));

    printf("Enter Nodes name\n");
    scanf("%s", network_name);

    char answer = '0';
    printf("Are you first node in network? [y/n] \n");
    scanf(" %c", &answer);

    nodes = create_array_list(NUMBER_OF_NODES);

    pthread_t udp_send_ping_thread;
    pthread_t udp_receive_ping_thread;
    pthread_t tcp_listen_thread;

    if (answer == 'n') {
        char *serv_ip="127.0.0.1",serv_port[IP_PORT_SIZE];
        printf("Please, main port of node inside network: \n");
        scanf("%s", serv_port);
        tcp_request_network_connection(serv_ip,serv_port);
    }
    else{
        node* first_node=malloc(sizeof(node));
        strcpy(first_node->ip , "127.0.0.1");
        strcpy(first_node->name,network_name);
        char port[IP_PORT_SIZE];
        snprintf(port,IP_PORT_SIZE,"%d",get_port(ping_fd));
        strcpy(first_node->ping_port,port);
        snprintf(port,IP_PORT_SIZE,"%d",get_port(main_sock));
        strcpy(first_node->port,port);
        array_list_add(nodes,first_node);
    }

    pthread_create(&udp_send_ping_thread, NULL, udp_send_ping, NULL);
    pthread_create(&udp_receive_ping_thread, NULL, udp_receive_ping, NULL);

    tcp_listen();
    return 0;
}
