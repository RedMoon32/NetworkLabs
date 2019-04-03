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
#include "shared/map.h"
#include <pthread.h>
#include <time.h>
#include <dirent.h>


#define IP_PORT_SIZE 16
#define NODE_NAME_SIZE 20

#define FILE_SIZE 2048
#define BUFF_SIZE 1024
#define MAX_NUMBER_OF_REQUESTS 10
#define BLOCKED -100

int SYNC = 1;
int REQUEST = 0;
map_int_t map;

typedef struct Node{
    char name[NODE_NAME_SIZE];
    char ip[IP_PORT_SIZE];
    char port[IP_PORT_SIZE];
    p_array_list files;
}node;

typedef struct Con_Info{
    int comm_sock_fd;
    char* key;
}con_info;

p_array_list nodes;
node me;
int main_sock;

node* node_in_list(char* ip,char *port){
    for (int i = array_list_iter(nodes);i!=-1;i=array_list_next(nodes,i)){
        node* current = array_list_get(nodes,i);
        if (strcmp(ip,current->ip)==0 & strcmp(port,current->port)==0)
            return current;
    }
    return NULL;
}

void get_file_names(p_array_list nfiles){
    DIR *d;
    struct dirent *dir;
    d = opendir("files");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name,".")==0|strcmp(dir->d_name,"..")==0)
                continue;
            char *new_file_name = malloc(strlen(dir->d_name));
            
            strcpy(new_file_name,dir->d_name);
            array_list_add(nfiles,new_file_name);
        }
        closedir(d);
    }
}

node* create_node(char* name,char* ip,char* port){
    node* new = malloc(sizeof(node));
    strcpy(new->ip,ip);
    strcpy(new->name,name);
    strcpy(new->port,port);
    new->files = create_array_list(10);
    return new;
}


void* connect_to_node(void* currentv){
    node* current = (node* )currentv;
    printf("Connecting to %s node\n",current->port);
    char buff[BUFF_SIZE];  
    struct sockaddr_in servaddr;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);   
    if (strcmp(current->ip,me.ip)==0&strcmp(current->port,me.port)==0)
        return;
    memset(&servaddr, 0, sizeof(servaddr));
    
    servaddr.sin_addr.s_addr = inet_addr(current->ip);
    servaddr.sin_family = AF_INET;
    uint16_t conn_port;
    sscanf(current->port,"%hu",&conn_port);
    servaddr.sin_port = htons(conn_port);
    
    if (connect(sock_fd,(struct sockaddr*)&servaddr,sizeof(servaddr))==-1){
        node* inlist = node_in_list(current->ip,current->port);
        printf("-1 connection\n");
        if (inlist!=NULL){
            printf("node deleted %s\n",current->name);
            array_list_remove(nodes,inlist);
        }
        return;
    }
    write(sock_fd,&SYNC,sizeof(int));
    //printf("=Sending %d\n",SYNC);
    buff[0]='\0';
    sprintf(buff,"%s:%s:%s:",me.name,me.ip,me.port);
    
    for (int i = array_list_iter(me.files);i!=-1;i=array_list_next(me.files,i)){
        char* current = array_list_get(me.files,i);
        strcat(buff,current);
        strcat(buff,",");
    }
    if (buff[strlen(buff)-1]==',')
        buff[strlen(buff)-1]='\0';
    //printf("=Sending %s\n",buff);
    write(sock_fd,buff,BUFF_SIZE);
    //close(sock_fd);//=====================================
    //return;
    buff[0] = '\0';
    sprintf(buff,"%ld",nodes->count);
    write(sock_fd,&nodes->count,sizeof(int));            
    //printf("=Sending %s\n",buff);
    for (int j = array_list_iter(nodes);j!=-1;j=array_list_next(nodes,j)){
        node* cur = array_list_get(nodes,j);
        buff[0]='\0';
        sprintf(buff,"%s:%s:%s",cur->name,cur->ip,cur->port);
        write(sock_fd,buff,BUFF_SIZE);
        //printf("=Sending %s\n",buff);
    }
    close(sock_fd);
    printf("Ping success\n");
}

void* syncing(){                                                                          
    while (1){
        array_list_free_all(me.files);
        get_file_names(me.files);

        for (int i = array_list_iter(nodes);i!=-1;i = array_list_next(nodes,i)){    
            pthread_t conn_thread;
            node* current = array_list_get(nodes,i);
            pthread_create(&conn_thread,NULL,connect_to_node,(void*)current);
            //connect_to_node(current);
        }
        sleep(5);   
    }
}   

int check_close(int n,con_info *data){
    printf("Number of bytes received:%d\n",n);
    if (n==-1||n==0){
            close(data->comm_sock_fd);
            int *con_count = map_get(&map,data->key);
            *con_count-=1;
            free(data);
            printf("Unexpected close\n");
            return 1;
    }
    return 0;
}

void process_request(con_info *data){
    char buff[BUFF_SIZE];
    int comm_sock_fd = data->comm_sock_fd;
    int status;
    int n = read(comm_sock_fd,&status,sizeof(int));
    int *con_count;
    check_close(n,data);
    printf("New ping processing\n");
    //printf("Got new message: %d \n",status);
    if (status==SYNC){
        //printf("==Starting to sync\n");
        n = read(comm_sock_fd,buff,BUFF_SIZE);
        if (check_close(n,data)) return ;
        //printf("==Received %s\n",buff);
        char* name = strtok(buff,":");
        char* ip = strtok(NULL,":");
        char* port = strtok(NULL, ":");
        char* files = strtok(NULL,":");
        node* new = node_in_list(ip,port);
        if (new == NULL){
            //printf("==New received node:%s %s\n",ip,port);
            new = create_node(name,ip,port);
            array_list_add(nodes,new);
        }
        else{
            array_list_free_all(new->files);  
        }
        //printf("==Got files:%s\n",files);
        char* file = strtok(files,",");
            
        while (file!=NULL){
            char* new_file = malloc(strlen(file));
            strcpy(new_file,file);
            array_list_add(new->files,new_file);
            strcpy(new_file,file);
            file = strtok(NULL,",");
        } 
        int count;
        n = read(comm_sock_fd,&count,sizeof(int));
        if (check_close(n,data)) return ;
        //printf("==Received %s\n",buff);
        //sscanf(buff,"%d",&count);
        for (int i = 0;i<count;i++){
            n = read(comm_sock_fd,buff,BUFF_SIZE);
            if (check_close(n,data)) return ;
            //printf("==Received node:%s\n",buff);
            char* name = strtok(buff,":");
            char* ip = strtok(NULL,":");
            char* port = strtok(NULL, ":");
            node* new = node_in_list(ip,port);
            if (new == NULL){
                array_list_add(nodes,create_node(name,ip,port));
                //printf("New node added from db:%s %s\n",ip,port);
            }
        }
        //int *con_count = map_get(&map,inet_ntoa())
    }
    else if (status==REQUEST){
        printf("Download Request\n");
        n = read(comm_sock_fd,buff,BUFF_SIZE);
        if (check_close(n,data)) return ;
        printf("File name:%s\n",buff);
        p_array_list temp_list = create_array_list(10);
        get_file_names(temp_list);
        for (int i = array_list_iter(temp_list); i!=-1; i = array_list_next(temp_list, i)){
            char* cur_file = array_list_get(temp_list,i);
            if (strcmp(cur_file,buff)==0){
                char text[FILE_SIZE];
                text[0] = '\0';
                int st = read_file_to_buffer(cur_file,text);
                if (st == -1){
                    printf("File not found\n");
                    write(comm_sock_fd,"-1",BUFF_SIZE);
                    break;
                }
                int count_of_words = 1;
                //Assume file is in following format: [word1_word2_word3_word4]
                
                for (int j = 0;j<strlen(text);j++){
                    if (text[j] == ' ' )
                        count_of_words+=1;
                }
                printf("Number of words in file: %d\n",count_of_words);
                sprintf(buff,"%d",count_of_words);
                write(comm_sock_fd,&count_of_words,sizeof(int));
                char* word = strtok(text," ");
                while (word!=NULL){
                    write(comm_sock_fd,word,BUFF_SIZE);
                    word = strtok(NULL," ");
                }
            }
        }
    }
    close(comm_sock_fd);
    con_count = map_get(&map,data->key);
    *con_count-=1;
    free(data);
    printf("Ping succ. processesd\n");
}

void* tcp_listen(){
    struct sockaddr_in cliaddr;
    int addr_len = sizeof(cliaddr);
    while (1){
        int comm_sock_fd = accept(main_sock,(struct sockaddr *) &cliaddr, &addr_len);
        if (comm_sock_fd == -1){
            printf("Accept");
            continue;
        }

        char *key = inet_ntoa(cliaddr.sin_addr);
        int *con_count = map_get(&map,key);
        
        if (con_count){
            printf("%d con_count from %s\n",*con_count,key);
            if (*con_count == BLOCKED){
                close(comm_sock_fd);
                continue;
            }
            else if (*con_count > MAX_NUMBER_OF_REQUESTS){
                *con_count = BLOCKED;
                close(comm_sock_fd);
                printf("%s was blocked\n",key);
                continue;
            }
            else{
                *con_count += 1;
            }
        }
        else{
            map_set(&map,key,1);
        }
        con_info* c = malloc(sizeof(con_info));
        c->key = malloc(IP_PORT_SIZE);
        strcpy(c->key,key);
        c->comm_sock_fd = comm_sock_fd;
        pthread_t new_conn;
        pthread_create(&new_conn,NULL,process_request,(void*)c);
        printf("Starting to process request\n");
        sleep(1);
    }
}

int read_file_to_buffer(char* file_name, char* buff){
    char file_path[256] = "files/";
    strcat(file_path,file_name);
    long length;
    FILE * f = fopen (file_path, "rb");
    if (f)
    {
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);
        fread (buff, 1, length, f);
        fclose (f);
        buff[length]='\0';
        return 0;
    }
    else{
        return -1;
    }
}

void download_from(node* server, char *file){
    struct sockaddr_in servaddr;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_addr.s_addr = inet_addr(server->ip);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(server->port));
    if (connect(sock_fd,(struct sockaddr*)&servaddr,sizeof(struct sockaddr))==-1){
        return;
    }
    char buff[BUFF_SIZE];
    strcpy(buff,file);
    write(sock_fd,&REQUEST,sizeof(int));
    write(sock_fd,buff,BUFF_SIZE);
    int count_of_words;
    read(sock_fd,&count_of_words,sizeof(int));
    
    //sscanf(buff,"%d",&count_of_words);
    printf("Count of words to receive - %d\n",count_of_words);
   
    if (count_of_words == -1){
        printf("File not found on the P2P node\n");
        return;
    }
    char file_path[256]="files/";strcat(file_path,file);
    FILE *fp = fopen (file_path, "w");
    
    for (int i = 0;i<count_of_words;i++){
        read(sock_fd,buff,BUFF_SIZE);
        fprintf(fp,"%s ",buff);
        printf("Got new Word:%s\n",buff);
    }
    fclose(fp);
    close(sock_fd);
}

void* download(char* file){
    printf("Searching for file in DB\n");
    for (int i = array_list_iter(nodes); i!=-1; i = array_list_next(nodes,i)){
        node* current = (node* ) array_list_get(nodes,i);
        if (strcmp(current->name,me.name)==0)
            continue;

        for (int j = array_list_iter(current->files); j!=-1; j = array_list_next(current->files,j)){
            char* nfile = (char* ) array_list_get(current->files,j);
            if (strcmp(nfile,file)==0){
                printf("File info found in db, downloading from %s\n",current->name);
                download_from(current,file);
                return;
            }
        }
    }
    printf("No file found in DB\n");
}

void ask_download(){
    char answer;
    printf("\nWanna download some files?\n");
    scanf(" %c", &answer);
    if (answer == 'y') {
        char file_name[32];
        printf("Enter the name of the file\n");
        scanf("%s",file_name);
        download(file_name);
        printf("Download ended\n");
    }
}

void init_main_socket(){
    main_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (main_sock == -1){
        perror("Socket error");
        exit(0);
    }
    if (listen(main_sock,100) == -1){
        perror("Listen error");
        exit(0);
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(main_sock, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else{
        snprintf(me.port,IP_PORT_SIZE,"%d",ntohs(sin.sin_port));
    }
}

void init(){
    nodes = create_array_list(10);
    printf("Enter name of the node:");
    scanf("%s",me.name);
    //printf("Enter ip address of some network you connected to (tryna use IFCONFIG):");
    //scanf("%s",me.ip);
    strcpy(me.ip,"127.0.0.1");
    init_main_socket();
    printf("Port you've been assigned to is:%s\n",me.port);
    char answer = '0';
    printf("Are you first node in network? [y/n]:");
    node* me_l = malloc(sizeof(node));
    memcpy(me_l,&me,sizeof(node));
    array_list_add(nodes,me_l);
    me.files = create_array_list(10);
    scanf(" %c", &answer);
    if (answer == 'n') {
        char ip[IP_PORT_SIZE] = "127.0.0.1";
        char port[IP_PORT_SIZE];
        //printf("Enter ip address of existing node:");
        //scanf("%s",ip);
        printf("Enter port address of existing node:");
        scanf("%s",port);
        node* new_node = create_node("F",ip,port);
        connect_to_node(new_node);
    }
}

int main(){
    init();
    map_init(&map);
    pthread_t send_sync_thread;
    pthread_t get_sync_and_req_thread;

    pthread_create(&send_sync_thread, NULL, syncing, NULL);
    pthread_create(&get_sync_and_req_thread,NULL, tcp_listen, NULL);
    printf("===[Background Syncing]===\n");
    //ask_download();
    pthread_join(get_sync_and_req_thread, NULL);

}