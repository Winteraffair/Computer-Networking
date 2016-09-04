all : arp

arp : main.o arp.o
	gcc main.o arp.o -o arp

main.o:main.c
	gcc -c main.c
arp.o:arp.c
	gcc -c arp.c

clean :
	rm -rf arp main.o arp.o *~

 
