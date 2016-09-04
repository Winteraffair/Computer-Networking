#ifndef __PCAP__H_
#define __PCAP__H_

#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/ip.h> 
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include "fill_packet.h"

#define FILTER_STRING_SIZE 100
extern char *net_c;
extern pid_t pid;
typedef struct{
	struct ether_header eth_hdr;
	myicmp header;
}pcap_packet;
void pcap_init( const char* dst_ip, int timeout );

int pcap_get_reply(int);

#endif
