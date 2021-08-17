#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

char state404[] = "HTTP/1.1 404 file not found\r\n\r\n";
char state200[] = "HTTP/1.1 200 ok\r\n";
int handle_request(int req);

int main(int argc, const char *argv[])
{
    int sock, req;
    struct sockaddr_in server, client;

    // create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create socket failed\n");
        return -1;
    }
    printf("socket created\n");

    // prepare the sockaddr_in structure
    int ip = atoi(argv[1]);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(ip);

    // bind
    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind failed\n");
        return -1;
    }
    printf("bind done\n");

    // listen
    listen(sock, 3);
    printf("waiting for incoming connections...\n");
    int c = sizeof(struct sockaddr_in);

    while (1)
    {
        // accept connection from an incoming client
        if ((req = accept(sock, (struct sockaddr *)&client, (socklen_t *)&c)) < 0)
        {
            perror("accept failed");
            continue;
        }
        printf("%d: connection accepted\n", req);

        //create thread for each client
        pthread_t *pid;
        pid = (pthread_t *)malloc(sizeof(pthread_t));
        pthread_create(pid, NULL, handle_request, (void *)req);
    }

    return 0;
}

int handle_request(int req)
{
    char msg[2000], ret_msg[4000] = {0};
    int msg_len = 0;
    int size = 0;
    FILE *fd;

    // receive a message from client
    while ((msg_len = recv(req, msg, sizeof(msg), 0)) > 0)
    {
        printf("msg_len: %d\n", msg_len);
        msg[msg_len] = 0;
        // send the message back to client
        // get file name
        strtok(msg, "/");
        char *file_name = strtok(NULL, " ");
        printf("file name: %s\n", file_name);

        // seek file
        if (!(fd = fopen(file_name, "r")))
        {
            printf("File not found\n");
            write(req, state404, strlen(state404));
            return -1;
        }
        fseek(fd, 0, SEEK_END);
        size = ftell(fd);
        //fclose(fd);

        //get file size
        char fsize[20];
        sprintf(fsize, "%d", size);
        printf("file size: %s\n", fsize);

        //get content
        char * buf = (char*) malloc(sizeof(char) * size);
        fseek(fd, 0, SEEK_SET);
        fread(buf, size + 1, sizeof(char), fd);

        //create header
        strcat(ret_msg, "HTTP/1.1 200 ok\r\n");
        strcat(ret_msg, "Content-type: text/plain\r\n");
        strcat(ret_msg, "Content-Length: ");
        strcat(ret_msg, fsize);
        strcat(ret_msg, "\r\n\r\n");
        //printf("%s\n", ret_msg);
        strcat(ret_msg, buf);
        //strcat(ret_msg, "\r\n");
        free(buf);

        write(req, ret_msg, strlen(ret_msg));
        fclose(fd);
    }
    printf("msg_len: %d\n", msg_len);

    if (msg_len == 0)
    {
        printf("client disconnected\n");
    }
    else
    {
        printf("recv failed\n");
        return -1;
    }
    return 0;
}