compile: client/client.c server/server.c common.c
	gcc client/client.c common.c -o remcp
	gcc server/server.c common.c -o remcp-serv -lpthread

clean:
	rm -rf remcp remcp-serv

run: compile
	./remcp-serv

build-server: server/server.c common.c
	gcc -g server/server.c common.c -o remcp-serv -lpthread


build-client: client/client.c common.c
	gcc -g client/client.c common.c -o remcp -lpthread