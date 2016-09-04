#include	"myftp.h"

int getIFname( int socketfd, char *device ) {
    //Function: To get the device name
    //Hint:     Use ioctl() with SIOCGIFCONF as an arguement to get the interface list
	struct ifconf ifconf;
	struct ifreq ifr[10];
	int i,ifc_len;
	
	ifconf.ifc_len = sizeof(ifr);
	ifconf.ifc_buf = (caddr_t)ifr;
	
	if(ioctl(socketfd,SIOCGIFCONF,&ifconf)==-1){
	  errCTL("ioctl");	
	}
	
	ifc_len = ifconf.ifc_len/sizeof(struct ifconf);
	for(i = 0 ; i< ifc_len;i++){
	  //printf("%s\n",ifr[i].ifr_name);
	  if(strcmp(ifr[i].ifr_name,"lo")!=0){
	    strcpy(device,ifr[i].ifr_name);
	    break;
	  }
	}
	

	return 0;
}

int initServAddr( int socketfd, int port, const char *device, struct sockaddr_in *addr ) {
    //Function: Bind device with socketfd
    //          Set sever address(struct sockaddr_in), and bind with socketfd
    //Hint:     Use setsockopt to bind the device
    //          Use bind to bind the server address(struct sockaddr_in)
	//bind the device
	struct ifreq ifr;

	if(setsockopt(socketfd,SOL_SOCKET,SO_BINDTODEVICE,device,strlen(device))==-1){
	  errCTL("setsockopt ser");	
	}

	addr->sin_family = AF_INET;
  	addr->sin_port = htons(port);
	//Get Device's ip
	strncpy(ifr.ifr_name,device,IFNAMSIZ);
	if(ioctl(socketfd,SIOCGIFADDR,&ifr)==-1){
	  errCTL("ioctl");	
	}
	addr->sin_addr.s_addr=htonl(INADDR_ANY);
  

	if(bind(socketfd,(struct sockaddr *)addr,sizeof(struct sockaddr_in))==-1){
	  errCTL("bind");	
	}
	addr->sin_addr=((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	//printf("%s\n",inet_ntoa(addr->sin_addr));

	printf("MyFtp Server start !\n");
	return 0;
}

int initCliAddr( int socketfd, int port, char *sendClient, struct sockaddr_in *addr ) {
    //Function: Set socketfd with broadcast option and the broadcast address(struct sockaddr_in)
    //Hint:     Use setsockopt to set broadcast option
	int i = 1;
	if(setsockopt(socketfd,SOL_SOCKET,SO_BROADCAST,&i,sizeof(int))==-1){
	  errCTL("setsockopt cli");
	}
	//printf("Port:%d\n",port);
	addr->sin_family = AF_INET;
  	addr->sin_port = htons(port);
  	inet_aton(sendClient,&addr->sin_addr);
	//printf("%s\n",inet_ntoa(addr->sin_addr));
	
	return 0;
}
int listenClient(int socketfd, int port, int *tempPort, char *filename, struct sockaddr_in *clientaddr , struct sockaddr_in *serveraddr){
	int revsize,block=0;
	int socklen = sizeof(struct sockaddr_in);
	struct bootServerInfo packet;
	struct ifreq ifr;

	srand(time(NULL));
	memset(&packet,'\0',sizeof(struct bootServerInfo));
	//printf("Port:%d\n",port);
	printf("share file : %s\n",filename);
	printf("wait client \n");

	/*clientaddr->sin_family = AF_INET;
  	clientaddr->sin_port = htons(port);*/
	while(block==0){
	  if((revsize =recvfrom(socketfd,&packet,sizeof(packet),0,(struct sockaddr *)clientaddr,&socklen))>0){
	    if(strcmp(filename , packet.filename)==0){
	      block =1;
	    }
	  }
	}
	printf("  [Request]\n     file : %s\n     from : %s\n",filename,inet_ntoa(clientaddr->sin_addr));
	//printf("clientaddr : %s\n",inet_ntoa(clientaddr->sin_addr));

	
	strcpy(packet.servAddr,inet_ntoa(serveraddr->sin_addr));
	*tempPort = port + (rand()%999+1);
	packet.connectPort = *tempPort ;
	if(sendto(socketfd,&packet,sizeof(packet),0,(struct sockaddr *)clientaddr,socklen)==-1){
	  errCTL("sendto"); 	
	}
	printf("  [Reply]\n     file : %s\n     servAddr : %s\n     connectPort:%d\n",filename,packet.servAddr,packet.connectPort);

	
	//set servaddr port, will use it to bind transmit port
	serveraddr->sin_port = htons(*tempPort);


	
	
}
int findServerAddr( int socketfd, char *filename, const struct sockaddr_in *broadaddr, struct sockaddr_in *servaddr ) {
    //Function: Send broadcast message to find server
    //          Set timeout to wait for server replay
    //Hint:     Use struct bootServerInfo as boradcast message
    //          Use setsockopt to set timeout
	struct bootServerInfo packet;
	struct timeval timeout;
	int revsize,block=0;
	int socklen=sizeof(struct sockaddr_in);
	
	memset(&packet,'\0',sizeof(struct bootServerInfo));
	timeout.tv_sec = 20;
	timeout.tv_usec = 0;
	if(setsockopt(socketfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==-1){
	  errCTL("setsockopt cli");
	}

	strcpy(packet.filename,filename);
	//printf("filename:%s\n",packet.filename);
	if(sendto(socketfd,&packet,sizeof(packet),0,(struct sockaddr *)broadaddr,socklen)==-1){
	  errCTL("sendto"); 	
	}
	while(block==0){
	  if((revsize =recvfrom(socketfd,&packet,sizeof(packet),0,(struct sockaddr *)servaddr,&socklen))>0){
	    if(strcmp(filename , packet.filename)==0){
	      block =1;
	    }
	  }
	  if(block==0){
	    printf("[TIME OUT]\n      No Server Answer,Send Broadcast Again\n");
	    if(sendto(socketfd,&packet,sizeof(packet),0,(struct sockaddr *)broadaddr,socklen)==-1){
	      errCTL("sendto"); 	
	    }
	  }
	}


	printf("  [Receive Apply]\n     Get MyFtp ServAddr : %s\n     MyFtp connectPort:%d\n",packet.servAddr,packet.connectPort);
	//set servaddr
	servaddr->sin_family = AF_INET;
	inet_aton(packet.servAddr,&servaddr->sin_addr);
	servaddr->sin_port = htons(packet.connectPort);
		
	return 0;
}

int startMyftpServer( int tempPort, struct sockaddr_in *clientaddr, const char *filename ,struct sockaddr_in *servaddr  ) {
	struct myFtphdr *frq = (struct myFtphdr *)malloc(strlen(filename)+5);
	struct myFtphdr *data = (struct myFtphdr *)malloc(sizeof(char)*512+6);
	struct myFtphdr *ack = (struct myFtphdr *)malloc(6);
	struct myFtphdr *error = (struct myFtphdr *)malloc(6);
	struct myFtphdr *final ;
	struct timeval timeout;
	int frqlen =strlen(filename)+5, datalen = sizeof(char)*512+6 , acklen=6,errorlen=6;
	int socklen = sizeof(struct sockaddr_in);
	int socketfd,revsize=0,i=0,block=1,numrec=0,end=1,testnumber=0;
	int length,count_block=0;
	FILE *in;
	//create transmit socket , and bind
	if((socketfd = socket(AF_INET,SOCK_DGRAM,0))==-1){
	  errCTL("socket");
	}
	if(bind(socketfd,(struct sockaddr *)servaddr,socklen)==-1){
	  errCTL("bind");	
	}
	//Set 20sec timeout
	timeout.tv_sec = 20;
	timeout.tv_usec = 0;
	if(setsockopt(socketfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==-1){
	  errCTL("setsockopt cli");
	}

	//recv frq packet and check;
	while(1){
	  if((revsize =recvfrom(socketfd,frq,frqlen,0,NULL,NULL))>0){
	    if(ntohs(frq->mf_opcode) == FRQ && in_cksum((unsigned short *)frq,frqlen)==0){
	      break;
	    }
	  }
	  else{ 	
	    	printf("  [Time Out]\n     wait client time out !\n");
		fflush(stdout);
	  }
	}
	//fopen client_filename
	in = fopen(filename,"r");
	if(in == NULL){
	   errCTL("fopen")	
	}
	//check how much blocks should be transmit
	fseek(in,0L,SEEK_END);
	length = ftell(in);
	numrec = length / 512 ;
	length = length % 512;
	fseek(in,0L,SEEK_SET);
	printf("  [file transmission - start]\n     sendfile <%s> to %s\n",filename,inet_ntoa(clientaddr->sin_addr));
	//send data
	data->mf_opcode = htons(DATA);
	ack->mf_opcode = htons(ACK);
	error->mf_opcode = htons(ERROR);
	while(end==1){
	  switch(block){
	    case 0:
		final = (struct myFtphdr *)malloc(length+6);
		data->mf_opcode = htons(DATA);
		data->mf_cksum = 0;
		data->mf_block = block;
		memset(data->mf_data,'\0',MFMAXDATA);
		fread(data->mf_data,length,1,in);
		data->mf_cksum = in_cksum((unsigned short *)data,datalen);
		//send data 
		if(sendto(socketfd,data,datalen,0,(struct sockaddr *)clientaddr,socklen)==-1){
	   	 errCTL("sendto"); 	
	 	}
		//recv client message
		if((revsize =recvfrom(socketfd,ack,acklen,0,NULL,NULL))>0){
	    	  if(ntohs(ack->mf_opcode) == ACK && in_cksum((unsigned short *)ack,acklen)==0){
			end = 0;
			//printf("end\n");
	          }
		  else if(ntohs(ack->mf_opcode) == ERROR){
		    fseek(in,-1*(length),SEEK_END);
		    printf("recv ERROR message ,send File again !\n");
		  }
		  else if(in_cksum((unsigned short *)ack,acklen)!=0){
		    printf("ACK ERROR , send ERROR to client\n");
		    error->mf_cksum = 0;
		    error->mf_block = block;
	            error->mf_cksum = in_cksum((unsigned short *)error,errorlen);
		    if(sendto(socketfd,data,datalen,0,(struct sockaddr *)clientaddr,socklen)==-1){
	   	      errCTL("sendto"); 	
	 	    }
		  }
	  	}
	  	else{ 	
	    		printf("  [Time Out]\n     wait client time out !\n ");
			fflush(stdout);
	 	}	
		break;
	    default:
		data->mf_cksum = htons(0);
		data->mf_block = block;
		fread(data->mf_data,MFMAXDATA,1,in);
		data->mf_cksum = in_cksum((unsigned short *)data,datalen);
		/*if(block==2015 && testnumber==0){
		  printf("test in 2015 error block send\n");
		  data->mf_cksum = 0;
		  data->mf_block = block+1;
		  fread(data->mf_data,MFMAXDATA,1,in);
		  data->mf_cksum = in_cksum((unsigned short *)data,datalen);
		  testnumber++;
		}*/
		//send data 
		if(sendto(socketfd,data,datalen,0,(struct sockaddr *)clientaddr,socklen)==-1){
	   	 errCTL("sendto"); 	
	 	}
		//recv client message
		if((revsize =recvfrom(socketfd,ack,acklen,0,NULL,NULL))>0){
	    	  if(ntohs(ack->mf_opcode) == ACK && in_cksum((unsigned short *)ack,acklen)==0){
	     	    block++;
		    count_block++;
	          }
		  else if(ntohs(ack->mf_opcode) == ERROR){
		    if(block == ack->mf_block){
		      printf("now block %d , ack block is %d\n",block,ack->mf_block);
		      fseek(in,(block-1)*MFMAXDATA,SEEK_SET);
		    }
		    else if(block <ack->mf_block){
		      printf("now block %d , ack block is %d\n",block,ack->mf_block);
		      printf("%ld\n",ftell(in)/512);
		      block++;
	    	      count_block++;
		      fseek(in,(block-1)*MFMAXDATA,SEEK_SET);		      
		    }
		    printf("recv ERROR message ,send packet again !\n");
		  }
		  else if(in_cksum((unsigned short *)ack,acklen)!=0){
		    printf("ACK ERROR , send ERROR to client\n");
		    error->mf_cksum = 0;
		    error->mf_block = block;
	            error->mf_cksum = in_cksum((unsigned short *)error,errorlen);
		    if(sendto(socketfd,data,datalen,0,(struct sockaddr *)clientaddr,socklen)==-1){
	   	      errCTL("sendto"); 	
	 	    }
		  }
	  	}
	  	else{ 	
	    		printf("  [Time Out]\n     wait client time out !\n ");
			//printf("block: %d\n",block);
			fflush(stdout);
			
	  	}
		if(count_block == numrec){
		  block = 0;
		}
		fflush(stdout);
		break;
	    case 65535:		
		data->mf_cksum = 0;
		data->mf_block = block;
		fread(data->mf_data,MFMAXDATA,1,in);
		data->mf_cksum = in_cksum((unsigned short *)data,datalen);
		//send data 
		if(sendto(socketfd,data,datalen,0,(struct sockaddr *)clientaddr,socklen)==-1){
	   	 errCTL("sendto"); 	
	 	}
		//recv client message
		if((revsize =recvfrom(socketfd,ack,acklen,0,NULL,NULL))>0){
	    	  if(ntohs(ack->mf_opcode) == ACK && in_cksum((unsigned short *)ack,acklen)==0){
	     	    block=1;
		    count_block++;
	          }
		  else if(ack->mf_opcode == ERROR){
		    if(block == ntohs(ack->mf_block)){
		      printf("now block %d , ack block is %d\n",block,ack->mf_block);
		      fseek(in,(block-1)*MFMAXDATA,SEEK_SET);
		    }
		    else if(block < ntohs(ack->mf_block)){
		      printf("now block %d , ack block is %d\n",block,ack->mf_block);
		      printf("%ld\n",ftell(in)/512);
		      block++;
	    	      count_block++;
		      fseek(in,(block-1)*MFMAXDATA,SEEK_SET);		      
		    }
		    printf("recv ERROR message ,send packet again !\n");
		  }
		  else if(in_cksum((unsigned short *)ack,acklen)!=0){
		    printf("ACK ERROR , send ERROR to client\n");
		    error->mf_cksum = 0;
		    error->mf_block = block;
	            error->mf_cksum = in_cksum((unsigned short *)error,errorlen);
		    if(sendto(socketfd,error,errorlen,0,(struct sockaddr *)clientaddr,socklen)==-1){
	   	      errCTL("sendto"); 	
	 	    }
		  }
	  	}
	 	else{ 	
	    		printf("  [Time Out]\n     wait client time out !\n ");
			fflush(stdout);
	  	}
		if(count_block == numrec){
		  block = 0;
		}
		break;	
	  }	
	}
	
	printf("  [file transmission - end]\n     transmit file <%s> : %d blocks %d byte (last : %d )\n",filename,count_block,numrec*512+length,length);
	fflush(stdout);

	exit(0);
}

int startMyftpClient( int socketfd, struct sockaddr_in *servaddr, const char *filename ) {
	struct myFtphdr *frq = (struct myFtphdr *)malloc(strlen(filename)+5);
	struct myFtphdr *data = (struct myFtphdr *)malloc(sizeof(char)*512+6);
	struct myFtphdr *ack = (struct myFtphdr *)malloc(6);
	struct myFtphdr *error = (struct myFtphdr *)malloc(6);
	int frqlen =strlen(filename)+5, datalen = sizeof(char)*512+6 , acklen=6,errorlen=6;
	int socklen = sizeof(struct sockaddr_in);
	int i=0,block=1,revsize=0,count_block=0,end=1,stop_point=0,timeout=0,testnumber=0;
	FILE *out;
	
	//close broadcast
	if(setsockopt(socketfd,SOL_SOCKET,SO_BROADCAST,&i,sizeof(int))==-1){
	  errCTL("setsockopt cli");
	}
	//open client_testfile
	system("rm -rf client_testfile client_testfile2");
	if(strcmp(filename,"testfile")==0)
	  out = fopen("client_testfile","w+");
	else if(strcmp(filename,"testfile2")==0)
	  out = fopen("client_testfile2","w+");
	if(out == NULL){
	  errCTL("fopen"); 	
	}
	frq->mf_opcode = htons(FRQ);
	ack->mf_opcode = htons(ACK);
	error->mf_opcode = htons(ERROR);
	//printf("frq = %d\n",frq->mf_opcode);
	frq->mf_cksum=0;
	strcpy(frq->mf_filename,filename);
	frq->mf_cksum = in_cksum((unsigned short *)frq,frqlen);
	sleep(1);
	while(1){
	  //send FRQ
	  if(sendto(socketfd,frq,frqlen,0,(struct sockaddr *)servaddr,socklen)==-1){
	    errCTL("sendto"); 	
	  }
	  //recv first data
	  if((revsize =recvfrom(socketfd,data,datalen,0,NULL,NULL))>0){
	    if(ntohs(data->mf_opcode) == DATA && in_cksum((unsigned short *)data,datalen)==0){
	      //printf("first data recv !!\n");
	      printf("  [file transmission - start]\n     downlaod file <%s> from %s \n",filename,inet_ntoa(servaddr->sin_addr));
	      ack->mf_cksum = 0;
	      ack->mf_block = htons(block);
	      ack->mf_cksum = in_cksum((unsigned short *)ack,acklen);
	      if(sendto(socketfd,ack,acklen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	 errCTL("sendto"); 	
	      }
	      block++;
	      count_block++;
	      fwrite(data->mf_data,MFMAXDATA,1,out);
	      fflush(stdout);
	      break;
	    }
	    else if(ntohs(data->mf_opcode) == ERROR){
	      ack->mf_cksum = 0;
	      ack->mf_block = htons(block);
	      ack->mf_cksum = in_cksum((unsigned short *)ack,acklen);
	      if(sendto(socketfd,ack,acklen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	errCTL("sendto"); 	
	      }
	      break;
	    }
	    else if(in_cksum((unsigned short *)data,datalen)!=0){
	      printf("DATA ERROR , send ERROR to Server\n");
	      error->mf_cksum = 0;
	      error->mf_block = htons(block);
	      error->mf_cksum = in_cksum((unsigned short *)error,errorlen);
	      if(sendto(socketfd,error,errorlen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	errCTL("sendto"); 	
	      }
	      break;
	    }
	  }
	  else{
	    printf("  [Time Out]\n     wait server time out !\n ");
	    fflush(stdout);	
	  }	  
	}
	//recv another data
	while(end==1){    
		if((revsize =recvfrom(socketfd,data,datalen,0,NULL,NULL))>0){
		  stop_point = strlen(data->mf_data);
		  if(stop_point < MFMAXDATA){
		   
		      block = 0;
		      fwrite(data->mf_data,stop_point/*88*/,1,out);
		      //printf("%d\n",data->mf_block);
		      count_block = count_block*512+stop_point/*88*/;
		      ack->mf_cksum = 0;
	              ack->mf_block = htons(block);
	              ack->mf_cksum = in_cksum((unsigned short *)ack,acklen);
		      if(sendto(socketfd,ack,acklen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	        errCTL("sendto"); 	
	              }
		      printf("  [file transmission - end]\n     downlaod file <%s> : %d byte <last : %d>\n",filename,count_block,stop_point);
		      end = 0;
		      break;
		  }
	          if(ntohs(data->mf_opcode) == DATA && in_cksum((unsigned short *)data,datalen)==0){
		    if(stop_point < MFMAXDATA){
		      block = 0;
		      fwrite(data->mf_data,stop_point,1,out);
		      printf("%d\n",count_block);
		      count_block = count_block*512+stop_point;
		      ack->mf_cksum = 0;
	              ack->mf_block = htons(block);
	              ack->mf_cksum = in_cksum((unsigned short *)ack,acklen);
		      if(sendto(socketfd,ack,acklen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	        errCTL("sendto"); 	
	              }
		      printf("  [file transmission - end]\n     downlaod file <%s> : %d byte <last : %d>\n",filename,count_block,stop_point);
		      end = 0;
		      break;
		    }
		    else{
		      //printf("block %d recv\n",data->mf_block);
		      if(data->mf_block == block /*&& timeout == 0*/){
			//if(ntohs(data->mf_block)==2){printf("2 recv\n");}
	                fwrite(data->mf_data,MFMAXDATA,1,out);
		        ack->mf_cksum = 0;
	                ack->mf_block = htons(block);
	                ack->mf_cksum = in_cksum((unsigned short *)ack,acklen);
		        if(sendto(socketfd,ack,acklen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	          errCTL("sendto"); 	
	                }
		        count_block++;
			block++;
			if(block == 65536){
			  block =1;			
			}
		      }
		      else{
			printf("Wrong block : should receive %d , but recv %d\n",block,data->mf_block);
	           	error->mf_cksum = 0;
	           	error->mf_block = htons(block);
	           	error->mf_cksum = in_cksum((unsigned short *)error,errorlen);
			if(sendto(socketfd,error,errorlen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	    	  errCTL("sendto"); 	
	           	}
		      }
		      /*if(data->mf_block == 2015 && testnumber==0){
				printf("ERROR TEST in 2015\n");
	            		count_block=0;
	            		fseek(out,0L,SEEK_SET);
	           		 error->mf_cksum = 0;
	           		 error->mf_block = block;
	           		 error->mf_cksum = in_cksum((unsigned short *)error,errorlen);
	            		if(sendto(socketfd,error,errorlen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	    		  errCTL("sendto"); 	
	           		 }
			testnumber=1;
			
		      }*/
		    }
	          }
	    	  else if(ntohs(data->mf_opcode) == ERROR){
	            printf("recv ERROR !!\n");
		    ack->mf_cksum = 0;
	            ack->mf_block = htons(block);
	            ack->mf_cksum = in_cksum((unsigned short *)ack,acklen);
		    if(sendto(socketfd,ack,acklen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	      errCTL("sendto"); 	
	            }
	          }
	          else if(in_cksum((unsigned short *)data,datalen)!=0){
	            printf("DATA ERROR , send ERROR to Server\n");
	            error->mf_cksum = 0;
	            error->mf_block = htons(block);
	            error->mf_cksum = in_cksum((unsigned short *)error,errorlen);
		    if(sendto(socketfd,error,errorlen,0,(struct sockaddr *)servaddr,socklen)==-1){
	   	      errCTL("sendto"); 	
	            }
	          }  
	        }
	  	else{
	   	 printf("  [Time Out]\n     wait server time out %d %d!\n",block,data->mf_block);
		 //printf("block : %d\n",block);
		 timeout=1;
		 
	 	}
		fflush(stdout);

	}
	
	
	return 0;
}

unsigned short in_cksum( unsigned short *addr, int len ) {
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;
	
	while( nleft > 1 ) {
		sum += *w++;
		nleft -= 2;
	}
	
	if( nleft == 1 ) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer;
	}
	
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}
