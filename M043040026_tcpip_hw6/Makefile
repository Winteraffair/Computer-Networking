all : myping

myping : main.c pcap.c fill_packet.c
	gcc -o myping main.c pcap.c fill_packet.c -lpcap

clean :
	rm -rf *~ *.o myping
