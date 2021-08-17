#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
 
int main(int argc, char *argv[])
{
    //Cut argument into ip,filename,port;
    if(argc != 2)
    {
        printf("Format:./http-client + ip:port/file\n");
        return -1;
    }
    char *arg;
    char ip[100] = {0};
    char portchar[100] = {0};
    char filename[100] = {0};
    FILE *fd;

    if((argv[1][0] == 'h')&&(argv[1][1] == 't')&&(argv[1][2] == 't')&&(argv[1][3] == 'p'))
        arg = argv[1] + 7;
    else
        arg = argv[1];

    
    int i = 0;
    while (arg[i] != ':')
    {
        ip[i] = arg[i];
        i++;
    } 
    ip[i] = '\0';
    i ++;
    int j = 0;
    while (arg[i] != '/')
    {
        portchar[j] = arg[i];
        i++;
        j++;
    }
    i++;
    portchar[j] = '\0';
    j = 0;
    int port = atoi(portchar);
    while (arg[i] != '\0')
    {
        filename[j] = arg[i];
        i ++;
        j ++;
    }
    filename[j] = '\0';
    printf("%s\n",ip);
    printf("%s\n",filename);
    printf("%d\n",port);


    int sock;
    struct sockaddr_in server;
    char message[1000]={0}, server_reply[2000];
     
    // create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("create socket failed");
		return -1;
    }
    printf("socket created");
     
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
 
    // connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed");
        return 1;
    }
    printf("connected\n");
    
    //create a http message
    strcat(message,"GET /");
    strcat(message,filename);
    strcat(message," HTTP/1.1\r\n");
    strcat(message,"User-Agent: ZXY's simple client\r\n");
    strcat(message,"Host: ");
    strcat(message,ip);
    strcat(message,"\r\nConnection:Keep-Alive");
    strcat(message,"\r\n\r\n");
    //printf("Http header: %s\n",message);

    // send some data
    if (send(sock, message, strlen(message), 0) < 0) {
        printf("send failed");
        return 1;
    }
    //generate a recv_file name
    char recv_file[2000] = {0};
    strcat(recv_file,filename);
    strcat(recv_file,"_recv");
    

    // receive a reply from the server
	int len = recv(sock, server_reply, 2000, 0);
    if (len < 0) {
        printf("recv failed");
    }
    //printf("%d\n",len);
	server_reply[len] = 0;

    char *sonchar;
    if ((sonchar = strstr(server_reply,"404 file not found"))!=NULL)
    {
        printf("File not found\n");
        return -1;
    }
    fd = fopen(recv_file, "w+");
    //find content length in the reply
    sonchar = strstr(server_reply,"Content-Length:");
    sonchar = sonchar + 15;
    int k = 0;
    int filesize = 0;
    while(sonchar[k] != '\r')
    {
        if(sonchar[k] >= '0' && sonchar[k] <= '9')
            filesize = filesize * 10 + sonchar[k] - '0';
        k ++;
    }
    //printf("%d\n",filesize);

    printf("server reply  \n");
    //printf("%s\n", server_reply);

    //find the end of header and then find the body
    sonchar = strstr(server_reply,"\r\n\r\n");
    sonchar = sonchar +4;
    fwrite(sonchar,filesize,1,fd);
    fclose(fd);
    close(sock);
    return 0;
}
