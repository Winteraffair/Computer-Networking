#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define socket_port 64026
void *client_recv(void *);
int leave_flag=0;
char myname[20];

int main(int argc , char *argv[]){
  int readsize;
  int socket_client;
  char message_client[100];
  char get_message[100];
  pthread_t thread_id;
  struct sockaddr_in serveraddr;
  int my_port,my_addr;

  

  if(argc <= 2){
    printf("Usage: ./client [ip] [port], please try again\n");
    exit(0);
  }
  my_port=atoi(argv[2]);
 
  if((socket_client=socket(AF_INET,SOCK_STREAM,0))==-1){
    ("Socket connect error");
    exit(0);
  }
  printf("Socket created\n");

  //set socketaddr
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(my_port);
  serveraddr.sin_addr.s_addr=inet_addr(argv[1]);

  if(connect(socket_client,(struct sockaddr *)&serveraddr,sizeof(serveraddr))==-1){
    perror("Connect server error");
    exit(0); 
  }
  printf("Server Connect\n");

  //get name and get room_table from server
  printf("[Welcome !]\n        Please enter your name:");
  gets(get_message);
  strcpy(myname,get_message);
  if(send(socket_client,get_message,strlen(get_message)+1,0) < 0){
    perror("send error");
    exit(0);
  }
  if((readsize = recv(socket_client,message_client,100,0)) > 0){
    printf("%s\n",message_client);
  }
  //get group
  printf("---------------------------\n        Please enter the group(if no exist, it will be created):");
  gets(get_message);
  if(send(socket_client,get_message,strlen(get_message)+1,0) < 0){
    perror("send error");
    exit(0);
  }
 
 
  printf("Usage : 1.[Message] - send to the room\n        2./w [user or room] [message] - send to the specific user or room\n        3.bye - leave\n");
  printf("---------------------------------\n");

  //create one thread to recv , main thread to send
  if(pthread_create(&thread_id,NULL,client_recv,(void *)&socket_client) < 0){
      perror("thread error");
      exit(0);
  }

  while(leave_flag==0){
    printf("[%s] >",myname);
    gets(get_message);
    if(strcmp(get_message,"bye")==0){
      leave_flag=1;
    }
    if(send(socket_client,get_message,strlen(get_message)+1,0) < 0){
      perror("send error");
      exit(0);
    }
   
  }
  close(socket_client);
}

void *client_recv(void *socket_client){
  int socket=*(int*)socket_client;
  int readsize;
  char message[100];
  

  while((readsize = recv(socket,message,100,0)) > 0){ 
      printf("%s",message);    
      printf("[%s] >",myname);
      fflush(stdout);
  }
}
