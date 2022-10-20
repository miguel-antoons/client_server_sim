client : client/client
	gcc -o client client/client_new.c 

server : server/server
	gcc -o server/server server/server.c

cleanClient : 
	$(RM) client/client

cleanServer :
	$(RM) server/server

