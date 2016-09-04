#include "pcap.h"
#include <sys/types.h>
#include <pcap/pcap.h>
#include <netinet/in.h>
#include <string.h>


extern pid_t pid;
extern u16 icmp_req;

static const char* dev = "eth0";
static char* net;
static char* mask;
char *net_c;
static char filter_string[FILTER_STRING_SIZE] = "";
struct in_addr addr_net;
static pcap_t *p;
static struct pcap_pkthdr hdr;

/*
 * This function is almost completed.
 * But you still need to edit the filter string.
 */
void pcap_init( const char* dst_ip ,int timeout )
{	
	int ret;
	char errbuf[PCAP_ERRBUF_SIZE];
	
	bpf_u_int32 netp;
	bpf_u_int32 maskp;
	
	struct in_addr addr;
	
	struct bpf_program fcode;

	ret = pcap_lookupnet(dev, &netp, &maskp, errbuf);
	if(ret == -1){
		fprintf(stderr,"%s\n",errbuf);
		exit(1);
	}
	
	addr.s_addr = netp;
	addr_net.s_addr = netp;
	net = inet_ntoa(addr);
	/*net_c = (char *)malloc(strlen(net)+1);	
	strcpy(net_c,net);
	printf("net_c = %s\n",net);*/
	if(net == NULL){
		perror("inet_ntoa");
		exit(1);
	}
	
	addr.s_addr = maskp;
	mask = inet_ntoa(addr);
	if(mask == NULL){
		perror("inet_ntoa");
		exit(1);
	}
	
	
	p = pcap_open_live(dev, 8000, 1, timeout, errbuf);
	if(!p){
		fprintf(stderr,"%s\n",errbuf);
		exit(1);
	}
	
	/*
	 *    you should complete your filter string before pcap_compile
	 */
	memset(filter_string,'\0',FILTER_STRING_SIZE);
	sprintf(filter_string,"%s%s","icmp and src host ",dst_ip);
	//sprintf(filter_string,"%s","icmp");
	//strcpy(filter_string,"'icmp && dst host 192.168.3.102'");

	if(pcap_compile(p, &fcode, filter_string, 0, maskp) == -1){
		pcap_perror(p,"pcap_compile");
		exit(1);
	}
	
	if(pcap_setfilter(p, &fcode) == -1){
		pcap_perror(p,"pcap_setfilter");
		exit(1);
	}
}


int pcap_get_reply(int sequence_num)
{
	const u_char *ptr;
	int i;
	struct timeval tm;
	ptr = pcap_next(p, &hdr);
	pcap_packet *packet = NULL;
	if(ptr != NULL){
	  packet = (pcap_packet *)(ptr-2) ;
	}
	//myicmp *packet = (myicmp *)ptr;
	/*
	 * google "pcap_next" to get more information
	 * and check the packet that ptr pointed to.
	 */
	while(ptr != NULL){
		//printf("p = %d , s = %d\n",pid,sequence_num);
		//printf("pid= %d,seq = %d\n",packet->header.icmp_hdr.un.echo.id,packet->header.icmp_hdr.un.echo.sequence);
		if(packet->header.icmp_hdr.un.echo.id == pid){
			if(packet->header.icmp_hdr.un.echo.sequence == sequence_num){
				return (int)(hdr.ts.tv_usec);			
			}		
		}
		//pcap_close(p);
		ptr = pcap_next(p, &hdr);
		if(ptr != NULL){
	  		packet = (pcap_packet *)(ptr-2) ;
		}
	}
	return 0;
	
}
