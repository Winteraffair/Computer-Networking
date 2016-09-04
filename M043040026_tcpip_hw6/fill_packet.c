#include "fill_packet.h"
#include "pcap.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>



void
fill_iphdr ( struct ip *ip_hdr , const char* dst_ip)
{

	ip_hdr->ip_hl = 0x07;
	ip_hdr->ip_v = 0x04;
	ip_hdr->ip_len = htons(PACKET_SIZE);
	ip_hdr->ip_id = 0;
	ip_hdr->ip_off = htons(IP_DF);
	ip_hdr->ip_ttl = 0x40;
	ip_hdr->ip_p = 0x01;
	//ip_hdr->ip_src.s_addr = ((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr;
	inet_aton(dst_ip,&(ip_hdr->ip_dst));
	
	
}

void
fill_icmphdr (struct icmphdr *icmp_hdr)
{
	extern pid_t pid;
	icmp_hdr->type = ICMP_ECHO;
	icmp_hdr->code = 0;
	icmp_hdr->checksum = 0;
	icmp_hdr->un.echo.id = pid;
	//icmp_hdr->un.echo.sequence = 0x01;
	icmp_hdr->checksum = fill_cksum(icmp_hdr);
}

u16
fill_cksum(struct icmphdr* icmp_hdr)
{	
	int nleft = ICMP_PACKET_SIZE;
	int sum = 0;
	unsigned short *w = (unsigned short *)icmp_hdr;
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
