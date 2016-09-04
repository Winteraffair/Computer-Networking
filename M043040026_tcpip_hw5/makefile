all : myftpServer myftpClient

myftpServer : myftp.c myftpServer.c
	gcc -o myftpServer myftp.c myftpServer.c
myftpClient : myftp.c myftpClient.c
	gcc -o myftpClient myftp.c myftpClient.c

clean : 
	rm -rf *.o *~ myftpServer myftpClient
