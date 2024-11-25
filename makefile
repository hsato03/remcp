compile: client.c server.c
	gcc client.c common.c -o client.out
	gcc server.c common.c -o server.out

clean:
	rm -rf *.out
