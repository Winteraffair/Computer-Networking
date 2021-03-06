#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define socket_port 64026

struct client{
  char name[20];
  char room[20];
  int  socket;
}client_table[20];

struct room{
  char room[20];
  char client[20][20];
  int amount;
}room_table[20];

int client_counter=0,room_counter=0;

void *client_thread(void *);



int main(int argc , char *argv[]){
  int c,leave_flag=0;
  int socket_server,socket_client;
  int check=0;
  char client[20][20];
  pthread_t thread_id;
  struct sockaddr_in serveraddr,clientaddr;
  int my_port;
  //set table 0
  for(c=0;c<20;c++){
    room_table[c].amount=0;
  }
 
  if(argc <= 1 || argc >=3){
    printf("Usage: ./server [port] , please try again\n");
    exit(0);
  }
  my_port=atoi(argv[1]);
  if((socket_server=socket(AF_INET,SOCK_STREAM,0))==-1){
    perror("Socket connect error");
    exit(0);
  }
  printf("Socket created\n");
  //set socketaddr
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(my_port);
  serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);

  //bind
  if(bind(socket_server,(struct sockaddr *)&serveraddr,sizeof(serveraddr))==-1){
    perror("bind error");
    exit(0);
  }
  printf("bind OK\n");

  //listen
  listen(socket_server,3);
  printf("------waiting for client------\n");

  //accept client
  c=sizeof(struct sockaddr_in);
  while(socket_client=accept(socket_server,(struct sockaddr *)&clientaddr,(socklen_t*)&c)){

    if(pthread_create(&thread_id,NULL,client_thread,(void *)&socket_client)<0){
      perror("thread error");
      exit(0);
    }
    //printf("thread establish\n");

    
  }
  

  

  close(socket_server);
  return 0;
}


void *client_thread(void *socket){
  char message_server[200],message_client[100];
  char myname[20],myroom[20],*blank=" ";
  int readsize,mycounter,myroomcounter,myamount;
  int socket_client=*(int*)socket;
  int i,j,check=0;
  int leave_flag=0;
  static char delim[]=" \t\n";
  char *token;
  
  
  printf("client connect:\n");
  /*strcpy(message_server,"[Welcome !]\nPlease enter your name:");
  if(send(*socket_client,message_server,strlen(message_server)+1,0) < 0){
    perror("send error");
    exit(0);
  }*/
  //get client name      
  if((readsize=recv(socket_client,message_client,100,0)) > 0){
    strcpy(myname,message_client);         
  }
  //sent room table
  if(room_counter==0){
    
    strcpy(message_server,"[room_table]\n        NULL");
    if(send(socket_client,message_server,strlen(message_server)+1,0) < 0){
      perror("send error");
      exit(0);
    }
   
  }
  else{
    for(i=0;i<room_counter;i++){
      for(j=0;j<room_table[i].amount;j++){
        if(i==0 && j==0){
          sprintf(message_server,"[room_table]\n        %s",room_table[i].client[j]);
        }
        else{
          if(j==0)
            sprintf(message_server,"%s%s",message_server,room_table[i].client[j]);
          else
            sprintf(message_server,"%s%s%s",message_server,blank,room_table[i].client[j]);
        }
      }
      sprintf(message_server,"%s%s[%s]\n        ",message_server,blank,room_table[i].room);
    }
    
    if(send(socket_client,message_server,strlen(message_server)+1,0) < 0){
      perror("send error");
      exit(0);
    }
  }
  //write client_table room_table
  if((readsize=recv(socket_client,message_client,100,0)) > 0){
    strcpy(client_table[client_counter].name,myname);
    client_table[client_counter].socket=socket_client; 
    strcpy(client_table[client_counter].room,message_client);  
    strcpy(myroom,message_client);
    client_counter++;

    for(i=0;i<room_counter;i++){
      if(strcmp(room_table[i].room,message_client)==0){
        strcpy(room_table[i].client[room_table[i].amount] , myname);
        myroomcounter = i;
        room_table[i].amount++;
        check=1;
        printf("the same room ! \n");
      }
    }
    if(check==0){
      strcpy(room_table[room_counter].room,message_client);
      strcpy(room_table[i].client[room_table[i].amount] , myname);
      myroomcounter = room_counter;
      room_counter++;
      room_table[i].amount++;
      printf("create new room:%s\n",room_table[i].room);
    }
   check=0;
  }
  //log in message
  printf("Client[%s] log in [%s] \n",myname,myroom);
  
  
  //send log in message to all users
  sprintf(message_server,"Client[%s] log in [%s]\n",myname,myroom);
  for(i=0;i<client_counter-1;i++){
    if(send(client_table[i].socket,message_server,strlen(message_server)+1,0) < 0){
      perror("send error");
      exit(0);
    }
  }
  
 
  while(leave_flag==0){
    if((readsize=recv(socket_client,message_client,100,0)) > 0){
      
      token=strtok(message_client,delim);
      // mycounter and myamount
      i=0;
      while(strcmp(client_table[i].name,myname)!=0){i++;}
      mycounter=i;
      i=0;
      while(strcmp(room_table[myroomcounter].client[i],myname)!=0){i++;}
      myamount=i;
      if(strcmp(token,"bye")==0){
        
        printf("Client[%s] leave chatroom",myname);
        leave_flag=1;
        //send leave message to all client
        sprintf(message_server,"Client[%s] leave chatroom\n",myname);
        for(i=0;i<client_counter;i++){
          if(i != mycounter){
            if(send(client_table[i].socket,message_server,strlen(message_server)+1,0) < 0){
              perror("send error");
              exit(0);
            }
          }
        }
        //client_table renew
        for(i=mycounter;i<=(client_counter-2);i++){
          strcpy(client_table[i].name,client_table[i+1].name);
          strcpy(client_table[i].room,client_table[i+1].room);
          client_table[i].socket=client_table[i+1].socket;
        }
        client_counter--;
        //room_table renew
        
        for(i=myamount;i<=(room_table[myroomcounter].amount-2);i++){
          strcpy(room_table[myroomcounter].client[i],room_table[myroomcounter].client[i+1]);
        }
        //room_table[myroomcounter].amount--;
        //if the room has no client , delete it.
        if(room_table[myroomcounter].amount==1){
          for(i=myroomcounter;i<=(room_counter-2);i++){
            strcpy(room_table[i].room,room_table[i+1].room);
            for(j=0;j<room_table[i].amount;j++){
              strcpy(room_table[i].client[j],room_table[i+1].client[j]);
            }
            room_table[i].amount=room_table[i+1].amount;
          }
          room_counter--;
        }
        
      }
      else if(strcmp(token,"/w")==0){
        check=0;
        token = strtok(NULL,delim);
        for(i=0;i<client_counter && check ==0;i++){
          if(strcmp(token,client_table[i].name)==0){
            check=1;
            token = strtok(NULL,delim);
            sprintf(message_server,"[%s whisper] : %s\n",myname,token);
            if(send(client_table[i].socket,message_server,strlen(message_server)+1,0) < 0){
             perror("send error");
             exit(0);
            }
          }
        }
        for(i=0;i<room_counter && check ==0;i++){
          if(strcmp(room_table[i].room,token)==0){
            check=1;
            token = strtok(NULL,delim);
            sprintf(message_server,"[%s][%s] : %s\n",myname,myroom,token);
            for(j=0;j<client_counter;j++){
              if(strcmp(room_table[i].room,client_table[j].room)==0){
                if(send(client_table[j].socket,message_server,strlen(message_server)+1,0) < 0){
                 perror("send error");
                 exit(0);
                }
              }
            }
          }
        }
        
      }
      //send message to clients in the same room
      else{
        sprintf(message_server,"[%s] : %s\n",myname,message_client);
        for(i=0;i<client_counter;i++){
          if(i != mycounter && strcmp(client_table[i].room,myroom)==0 ){
            if(send(client_table[i].socket,message_server,strlen(message_server)+1,0) < 0){
              perror("send error");
              exit(0);
            }
          }
        }
      }
    }    
  }
}
