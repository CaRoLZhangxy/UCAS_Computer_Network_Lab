all: http_client http_server

http_client: http_client.c
	gcc -Wall -g http_client.c -o http_client

http_server: http_server.c
	gcc -Wall -g http_server.c -o http_server -lpthread

clean:
	@rm -f http_client http_server
http-server: http-server.c
	gcc -Wall -g http-server.c -o http-server -lpthread