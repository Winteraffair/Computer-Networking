#include	"myftp.h"

// use ./myftpServer <port> <filename>
int main( int argc,char **argv ) {
	int socketfd;
	struct stat buf;
	struct sockaddr_in servaddr, clientaddr;
	char device[DEVICELEN];
	int tmpPort=44102,port;

	

	/* Usage information. */
	if( argc != 3 ) {
		printf( "usage: ./myftpServer <port> <filename>\n" );
		return 0;
	}
	port = atoi(argv[1]);
	/* Check if file exist. */
	if( lstat( argv[2], &buf ) < 0 ) {
		printf( "unknow file : %s\n", argv[2] );
		return 0;
	}

	/* Open socket. */
	if((socketfd = socket(AF_INET,SOCK_DGRAM,/*IPPROTO_UDP*/0))==-1){
		errCTL("socket");
	}

	/* Get NIC device name. */
	if( getIFname( socketfd, device ) )
		errCTL("getIFname");
	printf("Network Interface : %s\n",device);
	printf("Network Port : %d\n",port);
	/* Initial server address. */
	if( initServAddr(socketfd,port,device,&servaddr))
		errCTL("socket");  	
	

	//Function: Server can serve multiple clients
    //Hint: Use loop, listenClient(), startMyFtpServer(), and ( fork() or thread )
	while( 1 ) {
		listenClient(socketfd,port,&tmpPort,argv[2],&clientaddr,&servaddr);
		if( !fork() ) {
			startMyftpServer(tmpPort,&clientaddr,argv[2],&servaddr);
		}
	}

	return 0;
}
