compile: client.c server.c
	gcc client.c -o client.out
	gcc server.c -o server.out

clean:
	rm -rf *.out
