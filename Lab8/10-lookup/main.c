#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "basic.h"

long int space_cost = 0;
long int space_cost_2bit = 0;

int main() {
    printf("******************************Input**********************************\n");
    FILE * fd = fopen("forwarding-table.txt","r");
    
    char * line = (char*)malloc(MAXLINE);
    int input[MAXINPUT] = {0};
    int right_port[MAXINPUT] = {0};
    root = init_tree();
    root2 = init_tree_2bit();
    unsigned int ip_temp[4];
    int input_num = 0;
    while(fgets(line, MAXLINE, fd) != NULL) 
    {
        Info* new_info = (Info*)malloc(sizeof(Info));

        char *ip_char = strtok(line, " ");
        new_info->prefix_len = atoi(strtok(NULL," "));
        new_info->port = atoi(strtok(NULL," "));

        sscanf(ip_char,"%u.%u.%u.%u",&ip_temp[0],&ip_temp[1],&ip_temp[2],&ip_temp[3]);
        
        input[input_num] = ip_temp[0];
        input[input_num] = (input[input_num] << 8) + ip_temp[1];
        input[input_num] = (input[input_num] << 8) + ip_temp[2];
        input[input_num] = (input[input_num] << 8) + ip_temp[3];

        new_info->ip = input[input_num];
        insert_info(new_info);
        insert_info_2bit(new_info);
        input_num++;
    }
    
    printf("total mem 1bit: %ld\n", space_cost);
    printf("total mem 2bit: %ld\n", space_cost_2bit);
/*
    unsigned int ip1;
    while(1) 
    {
        scanf("%s",line);
        sscanf(line,"%u.%u.%u.%u",&ip_temp[0],&ip_temp[1],&ip_temp[2],&ip_temp[3]);
        ip1 = ip_temp[0];
        ip1 = (ip1 << 8) + ip_temp[1];
        ip1 = (ip1 << 8) + ip_temp[2];
        ip1 = (ip1 << 8) + ip_temp[3];
        printf("input:%x\n",ip1);
        printf("result port 1bit:%d\n",lookup(ip1));
        printf("result port 2bit:%d\n",lookup_2bit(ip1));
    }
    */
    fclose(fd);
    printf("******************************Check**********************************\n");
    fd = fopen("answer.txt","r");
    input_num = 0;
    while(fgets(line, MAXLINE, fd) != NULL) 
    {

        char *ip_char = strtok(line, " ");
        int prefix_len = atoi(strtok(NULL," "));

        right_port[input_num] = atoi(strtok(NULL," "));

        sscanf(ip_char,"%u.%u.%u.%u",&ip_temp[0],&ip_temp[1],&ip_temp[2],&ip_temp[3]);
        
        input[input_num] = ip_temp[0];
        input[input_num] = (input[input_num] << 8) + ip_temp[1];
        input[input_num] = (input[input_num] << 8) + ip_temp[2];
        input[input_num] = (input[input_num] << 8) + ip_temp[3];
        input_num++;
    }
    
    
    printf("input num: %d\n",input_num);
    int error = 0; 
    for (int i = 0; i < input_num; i ++)
    {   
        int r = lookup(input[i]);

        if(r!=lookup_2bit(input[i])||r!=right_port[i])
            error ++;
    }
    printf("error num :%d\n",error);
    
    printf("******************************Count**********************************\n");
    double time_use = 0;
    struct timeval start, end;
    int i = 0;
    gettimeofday(&start,NULL); 
    for (i = 0; i < input_num; i ++)
        lookup(input[i]);
    gettimeofday(&end,NULL);
    double timeuse = ( end.tv_sec - start.tv_sec ) * 1000000000 + (end.tv_usec - start.tv_usec) * 1000;  
    double num = (double)input_num;
    printf("1bit tree time_use is %.2lf ns\n",timeuse/num);

    gettimeofday(&start,NULL); 
    for (i = 0; i < input_num; i ++)
        lookup_2bit(input[i]);
    gettimeofday(&end,NULL);
    timeuse = ( end.tv_sec - start.tv_sec ) * 1000000000 + (end.tv_usec - start.tv_usec) * 1000;
    printf("2bit tree time_use is %.2lf ns\n",timeuse/num);
    return 0;
}


PrefixTree * init_tree() {
    PrefixTree * root = (PrefixTree *) malloc(sizeof(PrefixTree));
    space_cost += sizeof(PrefixTree);
    root->port = 0;
    return root;
}
PrefixTree2 * init_tree_2bit() {
    PrefixTree2 * root2 = (PrefixTree2 *) malloc(sizeof(PrefixTree2));
    space_cost_2bit += sizeof(PrefixTree2);
    root2->port = 0;
    return root2;
}

int insert_info (Info* new_info) 
{
    PrefixTree * ptr = root;
    unsigned int mask = 0x80000000;

    int insert_num = 0;
    for (int i = 0; i < new_info->prefix_len; i++) 
    {
        if ((new_info->ip & mask) == 0) 
        {
            if (ptr->left == NULL) 
            {
                PrefixTree *tmp = (PrefixTree *) malloc(sizeof(PrefixTree));
                space_cost += sizeof(PrefixTree);
                tmp->port = ptr->port;
                ptr->left = tmp;
            } 
            ptr = ptr->left;
        } 
        else 
        {
            if (ptr->right == NULL) 
            {
                PrefixTree *tmp = (PrefixTree *) malloc(sizeof(PrefixTree));
                space_cost += sizeof(PrefixTree);
                tmp->port = ptr->port;
                ptr->right = tmp;
            } 
            ptr = ptr->right;
        }
        mask = mask >> 1;
    }
    ptr->port = new_info->port;
    //free(new_info);
    return 1;
}

int insert_info_2bit (Info* new_info) {
    PrefixTree2 * ptr = root2;
    unsigned int mask = 0x80000000;
    unsigned int mask_next = 0x40000000; 

    int insert_num = 0;
    for (int i = 0; i < new_info->prefix_len-1; i+=2) 
    {
        if ((new_info->ip & mask) == 0) 
        {
            if((new_info->ip & mask_next) == 0)
            {
                if(!ptr->son00)
                {
                    PrefixTree2 *tmp = (PrefixTree2 *) malloc(sizeof(PrefixTree2));
                    space_cost_2bit += sizeof(PrefixTree2);
                    tmp->port = ptr->port;
                    ptr->son00 = tmp;
                }
                ptr = ptr->son00;
            }
            else
            {
                if(!ptr->son01)
                {
                    PrefixTree2 *tmp = (PrefixTree2 *) malloc(sizeof(PrefixTree2));
                    space_cost_2bit += sizeof(PrefixTree2);
                    tmp->port = ptr->port;
                    ptr->son01 = tmp;
                }
                ptr = ptr->son01;
            }
        } 
        else 
        {
            if((new_info->ip & mask_next) == 0)
            {
                if(!ptr->son10)
                {
                    PrefixTree2 *tmp = (PrefixTree2 *) malloc(sizeof(PrefixTree2));
                    space_cost_2bit += sizeof(PrefixTree2);
                    tmp->port = ptr->port;
                    ptr->son10 = tmp;
                }
                ptr = ptr->son10;
            }
            else
            {
                if(!ptr->son11)
                {
                    PrefixTree2 *tmp = (PrefixTree2 *) malloc(sizeof(PrefixTree2));
                    space_cost_2bit += sizeof(PrefixTree2);
                    tmp->port = ptr->port;
                    ptr->son11 = tmp;
                }
                ptr = ptr->son11;
            }
        }
        mask = mask >> 2;
        mask_next = mask_next >> 2;
    }
    if(new_info->prefix_len%2)
    {
        if((new_info->ip & mask) == 0)
        {
            if(!ptr->son00)
            {
                PrefixTree2 *tmp = (PrefixTree2 *) malloc(sizeof(PrefixTree2));
                space_cost_2bit += sizeof(PrefixTree2);
                tmp->port = new_info->port;
                ptr->son00 = tmp;
            }
            if (!ptr->son01)
            {
                PrefixTree2 *tmp = (PrefixTree2 *) malloc(sizeof(PrefixTree2));
                space_cost_2bit += sizeof(PrefixTree2);
                tmp->port = new_info->port;
                ptr->son01 = tmp;
            }
        }
        else
        {
            if(!ptr->son10)
            {
                PrefixTree2 *tmp = (PrefixTree2 *) malloc(sizeof(PrefixTree2));
                space_cost_2bit += sizeof(PrefixTree2);
                tmp->port = new_info->port;
                ptr->son10 = tmp;
            }
            if (!ptr->son11)
            {
                PrefixTree2 *tmp = (PrefixTree2 *) malloc(sizeof(PrefixTree2));
                space_cost_2bit += sizeof(PrefixTree2);
                tmp->port = new_info->port;
                ptr->son11 = tmp;
            }
        }
    }
    else
        ptr->port = new_info->port;
    free(new_info);
    return 1;
}

int lookup (int ip) 
{
    PrefixTree * ptr = root;
    PrefixTree * ret = NULL;
    unsigned int mask = 0x80000000;
    int i;
    for (i = 1; ptr; i++) 
    {
        ret = ptr;
        if ((ip & mask) == 0) 
            ptr = ptr->left;
        else 
            ptr = ptr->right;
        mask = mask >> 1;
    }
    return ret->port;
}
int lookup_2bit (int ip) 
{
    PrefixTree2 * ptr = root2;
    PrefixTree2 * ret = NULL;
    unsigned int mask = 0x80000000;
    unsigned int mask_next = 0x40000000;
    int i;
    for (i = 1; ptr; i +=2) 
    {
        ret = ptr;
        if ((ip & mask) == 0) 
        {
            if((ip & mask_next) == 0)
                ptr = ptr->son00;
            else
                ptr = ptr->son01;
        } 
        else 
        {
            if((ip & mask_next) == 0)
                ptr = ptr->son10;
            else
                ptr = ptr->son11;
        }
        mask = mask >> 2;
        mask_next = mask_next >> 2;
    }
    return ret->port;
}