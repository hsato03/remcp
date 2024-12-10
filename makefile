compile: client/client.c server/server.c common.c
	gcc client/client.c common.c -o client.out
	gcc server/server.c common.c -o server.out -lpthread

clean:
	rm -rf *.out

run: compile
	./server.out

build-server: server/server.c common.c
	gcc -g server/server.c common.c -o server.out -lpthread


build-client: client/client.c common.c
	gcc -g client/client.c common.c -o client.out -lpthread