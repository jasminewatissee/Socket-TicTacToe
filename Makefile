all : client server

clean: 
	rm client client.o
	rm server server.o

#### CLIENT

client : client.o
	cc -g -o client client.o

client.o : client.c
	cc -g  -Wall -o client.o -c client.c

clean-client: 
	rm client client.o
  
#### SERVER

server : server.o
	cc -g -o server server.o

server.o : server.c
	cc -g  -Wall -o server.o -c server.c

clean-server: 
	rm server server.o