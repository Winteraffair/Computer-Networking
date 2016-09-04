#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>

#include "fill_packet.h"
#include "pcap.h"


pid_t pid;
void if_ip(char *, char*);
static const char* dev = "eth0";

int main(int argc, char* argv[])
{
	int sockfd;
	int on = 1,i,sequence_num=1,pcap_check=0;
	char c,*target_ip,*copy_target_ip,*gateway,data[56];
	struct timeval tm;

	pid = getpid();
	struct sockaddr_in dst;
	struct ifreq req;
	myicmp *packet = (myicmp*)malloc(PACKET_SIZE);
	int count = DEFAULT_SEND_COUNT;
	int timeout = DEFAULT_TIMEOUT;
	
	/* 
	 * in pcap.c, initialize the pcap
	 */
	while((c = getopt(argc,argv,"g:w:c:"))!=-1){
		switch(c){
			case 'g':
				gateway = malloc(strlen(optarg)+1);
				strcpy(gateway,optarg);
				break;
			case 'w':
				timeout = atoi(optarg);
				break;
			case 'c':
				count = atoi(optarg);
				break;
			default :
				printf("Usage ./myping -g [gateway] -w [timeout] -c [count]\n");
				exit(0);		
		}
	}	
	target_ip = (char *)malloc(strlen(argv[optind])+1);
	copy_target_ip = (char *)malloc(strlen(argv[optind])+1);
	strcpy(target_ip,argv[optind]);
	strcpy(copy_target_ip,target_ip);
	//printf("target_ip:%s\ngateway:%s\npacket_size:%ld\n",target_ip,gateway,sizeof(myicmp));
	pcap_init( copy_target_ip , timeout);

	
	if((sockfd = socket(AF_INET, SOCK_RAW , IPPROTO_RAW)) < 0)
	{
		perror("socket");
		exit(1);
	}

	if(setsockopt( sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
	{
		perror("setsockopt");
		exit(1);
	}
	//get self_ip and fill in packt src
	strncpy(req.ifr_name,dev,IFNAMSIZ);
	if(ioctl(sockfd,SIOCGIFADDR,&req)<0){
		perror("ioctl");
		exit(1);         
        }
	packet->ip_hdr.ip_src = ((struct sockaddr_in *)&req.ifr_addr)->sin_addr;
	//printf("source ip : %s\n",inet_ntoa(packet->ip_hdr.ip_src));
	//fill packet
	fill_iphdr(&(packet->ip_hdr),gateway);
	fill_icmphdr(&(packet->icmp_hdr));
	//dst_addr set
	dst.sin_family = AF_INET;
	inet_aton(gateway,&dst.sin_addr);
	//icmp payload
	memset(packet->data,'\0',56);
	strcpy(packet->data,"Data");
	//ip_option
	packet->ip_option[0] = 0x83;
	packet->ip_option[1] = 0x07;
	packet->ip_option[2] = 0x04;
	if_ip(packet->ip_option+3,target_ip);
	//printf("packet : %d.%d.%d.%d\n",packet->ip_option[3],packet->ip_option[4],packet->ip_option[5],packet->ip_option[6]);
	packet->ip_option[7] = 0;
	/*
	 *   Use "sendto" to send packets, and use "pcap_get_reply"(in pcap.c) 
 	 *   to get the "ICMP echo response" packets.
	 *	 You should reset the timer every time before you send a packet.
	 */
	printf("\nPing %s (data_size = 56 , id = 0x%x , timeout = %d ,count = %d) : \n",copy_target_ip,pid,timeout,count);
	for(i=1;i<=count;i++){
		//fill sequence_num and get checksum
		packet->icmp_hdr.un.echo.sequence = sequence_num;
		fill_cksum(&(packet->icmp_hdr));
		//send packet
	 	if(sendto(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *)&dst, sizeof(dst)) < 0)
		{
			perror("sendto");
			exit(1);
		}
		gettimeofday(&tm,NULL);
		if((pcap_check = pcap_get_reply(sequence_num)) != 0 ){
			printf("Reply from %s = %.3f ms\n",copy_target_ip,/*(tm.tv_sec*1000 + */(double)(pcap_check - tm.tv_usec)/1000);
			printf("      Router : %s\n",gateway);		
		}
		else{
			printf("Reply from %s = *\n",copy_target_ip);		
		}
		sequence_num++;
		fflush(stdout);
	}

	free(packet);

	return 0;
}
void if_ip(char *incoming_ip , char* ip){
   char delim[] = ". \n\t";
   char *token=0; 
   int i;
               
   for(i=0;i<=4;i++){
      if(i==0)
        token = strtok(ip,delim);
      else if(i<=3){
        token = strtok(NULL,delim);
        if(token == NULL){
          printf("Wrong filter ip address\n");
          exit(1);
        }
      }
      else{
        token = strtok(NULL,delim);
        if(token != NULL){
          printf("Wrong filter ip address\n");
          exit(1);
        }
                    }
      if(i<=3){
        if(atoi(token)>=0 && atoi(token) <256 ){ 
           incoming_ip[i]=strtol(token,NULL,0);
        }
        else{
          printf("Wrong filter ip address\n");
          exit(0);
        }
     }
  }
}

