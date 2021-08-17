#include "tcp_sock.h"

#include "log.h"

#include <stdlib.h>
#include <unistd.h>

// tcp server application, listens to port (specified by arg) and serves only one
// connection request
void *tcp_server(void *arg)
{
	u16 port = *(u16 *)arg;
	struct tcp_sock *tsk = alloc_tcp_sock();

	struct sock_addr addr;
	addr.ip = htonl(0);
	addr.port = port;
	if (tcp_sock_bind(tsk, &addr) < 0) {
		log(ERROR, "tcp_sock bind to port %hu failed", ntohs(port));
		exit(1);
	}

	if (tcp_sock_listen(tsk, 3) < 0) {
		log(ERROR, "tcp_sock listen failed");
		exit(1);
	}

	log(DEBUG, "listen to port %hu.", ntohs(port));

	struct tcp_sock *csk = tcp_sock_accept(tsk);

	log(DEBUG, "accept a connection.");
	int wsize = 0;
	FILE *fd;
	fd = fopen("server-output.dat","wb");
	char rbuf[10001];
	int rlen = 0;
	while (1) {
		rlen = tcp_sock_read(csk, rbuf, 10000);
		if (rlen == 0) {
			log(DEBUG, "tcp_sock_read return 0, finish transmission.");
			break;
		} 
		else if (rlen > 0) {
			wsize += fwrite(rbuf, 1, rlen, fd);
			fflush(fd);
			printf("Received %d bytes + %d\n", wsize,rlen);
		}
		else {
			log(DEBUG, "tcp_sock_read return negative value, something goes wrong.");
			exit(1);
		}
	}
	fclose(fd);
	log(DEBUG, "close this connection.");

	tcp_sock_close(csk);
	
	return NULL;
}

// tcp client application, connects to server (ip:port specified by arg), each
// time sends one bulk of data and receives one bulk of data 
void *tcp_client(void *arg)
{
	struct sock_addr *skaddr = arg;

	struct tcp_sock *tsk = alloc_tcp_sock();

	if (tcp_sock_connect(tsk, skaddr) < 0) {
		log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", \
				NET_IP_FMT_STR(skaddr->ip), ntohs(skaddr->port));
		exit(1);
	}
	int wsize = 0;
	FILE *fd;
	fd = fopen("client-input.dat", "r");
	char *wbuf = malloc(10001);

	while(!feof(fd)) 
	{
		int wflen = fread(wbuf, 1, 10000, fd);	
		int wlen = tcp_sock_write(tsk, wbuf, wflen) ;
		if(wlen < 0)
			break;

		wsize += wlen;
		
		printf("Sent %d bytes\n", wsize);

		usleep(10000);
	}

	tcp_sock_close(tsk);

	free(wbuf);

	return NULL;
}
