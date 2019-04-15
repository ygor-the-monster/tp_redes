all: client server

client:
	gcc source/client.c -o bin/navegador

server:
	gcc source/server.c source/sds.c -o bin/servidor -lpthread

clear:
	rm bin/*
