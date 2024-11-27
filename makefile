compile: client/client.c server/server.c common.c
	gcc client/client.c common.c -o client.out
	gcc server/server.c common.c -o server.out

clean:
	rm -rf *.out
