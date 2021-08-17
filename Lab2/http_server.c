#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
int func_server(int sock); 
int* GetPrefixValue(char* strPattern, int iPatternLen);
int KMPStringMatch(char* strPattern, int iPatternLen, char* strTarget, int iTargetLen, int* prefix);
int KMP(char* strPattern, char* strTarget);
int file_size(char* filename);
int main(int argc, const char *argv[])
{
    int s, cs;
    struct sockaddr_in server, client;
   
    
    int port= atoi(argv[1]);//get port number

    // create socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket failed");
		return -1;
    }
    printf("socket created\n");
    // prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
     
    // bind
    if (bind(s,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return -1;
    }
    printf("bind done\n");
    // listen
    listen(s, 8);
    printf("waiting for incoming connections...\n");
    fflush(stdout);
    // accept connection from an incoming client
    while(1)
    {
        int c = sizeof(struct sockaddr_in);
        if ((cs = accept(s, (struct sockaddr *)&client, (socklen_t *)&c)) < 0) 
        {
            printf("accept failed\n");
            continue;
        }
        //create a thread for each client
        pthread_t *pid;
        pid = (pthread_t *)malloc(sizeof(pthread_t));
        pthread_create(pid, NULL, (void *)func_server, (void *)cs);
    }
    return 0;
}
int func_server(int sock)
{
    FILE *fd;
    char msg[2000] = {0};
    char file_content[2000] = {0};
    char ret_msg[200] = {0};
    printf("connection accepted\n");
    fflush(stdout);
    int msg_len = 0;
    char filename[200] = {0};
        // receive a message from client
    while ((msg_len = recv(sock, msg, sizeof(msg), 0)) > 0) 
    {
        // send the message back to client
        msg[msg_len] = 0;
        //printf("%s\n",msg);
        char *son_str;
        //find file name
        son_str = strstr(msg,"GET /");
        son_str = son_str + 5;
        int k = 0;
        //printf("son : %s\n",son_str);
        //find the end of file name
        k = KMP(son_str," HTTP/1.1\r\n");
        filename[k] = 0;
        int x = 0;
        for (x = 0;x < k;x ++)
            filename[x] = son_str[x];
        //printf("filename : %s\n",filename);

        if((fd = fopen(filename, "r"))==NULL)//can't find the file
        {
            printf("No such file.\n");
            write(sock,"HTTP/1.1 404 file not found\r\n\r\n",strlen("HTTP/1.1 404 file not found\r\n\r\n"));
            return -1;
        } 
        fclose(fd);
        //get the filesize of a file
        int filesize = file_size(filename);
        //printf("filesize:%d\n",filesize);   
        char size_str[200] = {0};
        sprintf(size_str, "%d", filesize);//make a int into string in a Linux way
        //create a return message header
        strcat(ret_msg, "HTTP/1.1 200 ok\r\n");
        strcat(ret_msg,"Server: ZXY's simple http server\r\n");
        strcat(ret_msg, "Content-Type: text/plain\r\n");
        strcat(ret_msg, "Content-Length: ");
        strcat(ret_msg, size_str);
        strcat(ret_msg, "\r\n\r\n");

        fd = fopen(filename, "r");
        //get file content
        while (!feof(fd))
        {
            memset(file_content,0,sizeof(msg));    
            fgets(file_content, sizeof(file_content), fd);
            //printf("send-msg:%s\n",file_content);
            strcat(ret_msg, file_content);
        }
        
        write(sock, ret_msg, strlen(ret_msg));

        
        

        fclose(fd);

    }

        if (msg_len == 0) {
            printf("client disconnected");
        }
        else { // msg_len < 0
            perror("recv failed");
		    return -1;
        }
        fflush(stdout);
        return 0;
}
//获得prefix数组
int* GetPrefixValue(char* strPattern, int iPatternLen)
{
    int i, j; /* i runs through the string, j counts the hits*/
    int* prefix = (int*)malloc(iPatternLen*sizeof(int));
   
    i = 1; j = 0;
    prefix[0] = 0;
   
    while(i<iPatternLen)
    {
        if(strPattern[i] == strPattern[j])
        {
            prefix[i] = ++j;
        }
        else
        {
            j = 0;
            prefix[i] = j;
        }
       
        i++;
    }
   
    return prefix;          
}   

//返回target串在pattern串中第一次匹配的index
int KMPStringMatch(char* strPattern, int iPatternLen, char* strTarget, int iTargetLen, int* prefix)
{
    int i = 0;
    int j = 0;
   
    while(i<iPatternLen && j<iTargetLen)
    {
        if(j==0 || strPattern[i]==strTarget[j])
        {
            i++;  j++;
        }
        else
        {
            j = prefix[j];
        }
    }            
   
    free(prefix);
    
    if(j==iTargetLen)
    {
        return i-j;
    }
    else
    {
        return -1;
    }        
}        
//KMP字符串匹配算法
int KMP(char* strPattern, char* strTarget)
{
    int* prefix = GetPrefixValue(strPattern, strlen(strPattern));
    int index = KMPStringMatch(strPattern, strlen(strPattern), strTarget, strlen(strTarget), prefix);
    return index;
}
//获取文件大小
int file_size(char* filename)
{
    FILE *fp=fopen(filename,"r");
    if(!fp) return -1;
    fseek(fp,0L,SEEK_END);
    int size=ftell(fp);
    fclose(fp);
    printf("%d\n",size);
    return size;
}
