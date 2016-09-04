#include	"myftp.h"

// use ./myftpClient <port> <filename>
int main( int argc, char **argv ) {
	int socketfd;
	struct sockaddr_in servaddr,broadaddr;
	int result;
	int port;
	char broadcast_mininet[] = "255.255.255.255";

	/* Usage information.	*/
	if( argc != 3 ) {
		printf( "usage: ./myftpClient <port> <filename>\n" );
		return 0;
	}
	port = atoi(argv[1]);
	/* Open socket.	*/
	if((socketfd = socket(AF_INET,SOCK_DGRAM,/*IPPROTO_UDP*/0))==-1){
		errCTL("socket");
	}
	/* Initial client address information. */
	if( initCliAddr(socketfd,port,broadcast_mininet,&broadaddr))
		errCTL("socket");
	
	/* Find server address.	*/
	/* & set time out.		*/
	result = findServerAddr(socketfd,argv[2], &broadaddr, &servaddr );
	
	/* Start ftp client */
	if( startMyftpClient(socketfd,&servaddr,argv[2])){
	  	errCTL("socket");
	}
	
	return 0;
}
