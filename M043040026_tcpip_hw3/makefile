all : server client
server:server_socket.o
	gcc server_socket.o -o server -lpthread
client:client_socket.o
	gcc client_socket.o -o client -lpthread

server_socket.o:server_socket.c
	gcc -c server_socket.c 
client_socket.o:client_socket.c
	gcc -c client_socket.c

clean:
	rm -rf server client server_socket.o client_socket.o

