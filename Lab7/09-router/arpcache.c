#include "arpcache.h"
#include "arp.h"
#include "ether.h"
#include "icmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static arpcache_t arpcache;

// initialize IP->mac mapping, request list, lock and sweeping thread
void arpcache_init()
{
	bzero(&arpcache, sizeof(arpcache_t));

	init_list_head(&(arpcache.req_list));

	pthread_mutex_init(&arpcache.lock, NULL);

	pthread_create(&arpcache.thread, NULL, arpcache_sweep, NULL);
}

// release all the resources when exiting
void arpcache_destroy()
{
	pthread_mutex_lock(&arpcache.lock);

	struct arp_req *req_entry = NULL, *req_q;
	list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list) {
		struct cached_pkt *pkt_entry = NULL, *pkt_q;
		list_for_each_entry_safe(pkt_entry, pkt_q, &(req_entry->cached_packets), list) {
			list_delete_entry(&(pkt_entry->list));
			free(pkt_entry->packet);
			free(pkt_entry);
		}

		list_delete_entry(&(req_entry->list));
		free(req_entry);
	}

	pthread_kill(arpcache.thread, SIGTERM);

	pthread_mutex_unlock(&arpcache.lock);
}

// lookup the IP->mac mapping
//
// traverse the table to find whether there is an entry with the same IP
// and mac address with the given arguments
int arpcache_lookup(u32 ip4, u8 mac[ETH_ALEN])
{
	//fprintf(stderr, "TODO: lookup ip address in arp cache.\n");
	pthread_mutex_lock(&arpcache.lock);//获取互斥锁，保证对缓存的操作是互斥的
	for(int i = 0; i < MAX_ARP_SIZE; i++)
	{
		if(arpcache.entries[i].ip4 == ip4 && arpcache.entries[i].valid)//找到对应表项
		{
			memcpy(mac,arpcache.entries[i].mac,ETH_ALEN);//将找到的mac地址存入，释放互斥锁并返回
			pthread_mutex_unlock(&arpcache.lock);
			return 1;
		}
	}
	pthread_mutex_unlock(&arpcache.lock);
	return 0;
}

// append the packet to arpcache
//
// Lookup in the list which stores pending packets, if there is already an
// entry with the same IP address and iface (which means the corresponding arp
// request has been sent out), just append this packet at the tail of that entry
// (the entry may contain more than one packet); otherwise, malloc a new entry
// with the given IP address and iface, append the packet, and send arp request.
void arpcache_append_packet(iface_info_t *iface, u32 ip4, char *packet, int len)
{
	//fprintf(stderr, "TODO: append the ip address if lookup failed, and send arp request if necessary.\n");
	pthread_mutex_lock(&arpcache.lock);
	int uncached = 1;//标志请求的ip是否已经在缓存中
	struct arp_req *entry,*q;
	struct cached_pkt *pkt = (struct cached_pkt *)malloc(sizeof(struct cached_pkt));//生成待发送的数据包
	pkt->len = len;
	pkt->packet = packet;
	init_list_head(&pkt->list);
	list_for_each_entry_safe(entry, q, &arpcache.req_list,list)
	{
		if(entry->ip4 == ip4)//在缓存中找到了请求的ip
		{
			list_add_tail(&pkt->list,&(entry->cached_packets));
			uncached = 0;
			break;
		}
	}
	if(uncached == 1)//如果不在缓存中，则新生成一个req项添加到队尾，并将数据包加入，进入等待发送状态
	{
		struct arp_req *new_req = (struct arp_req *)malloc(sizeof(struct arp_req));
		init_list_head(&new_req->list);
		new_req->iface = iface;
		new_req->ip4 = ip4;
		new_req->sent = time(NULL);
		new_req->retries = 0;
		init_list_head(&new_req->cached_packets);
		list_add_tail(&new_req->list,&arpcache.req_list);
		list_add_tail(&pkt->list,&new_req->cached_packets);
		arp_send_request(iface,ip4);

	}
	pthread_mutex_unlock(&arpcache.lock);
}

// insert the IP->mac mapping into arpcache, if there are pending packets
// waiting for this mapping, fill the ethernet header for each of them, and send
// them out
void arpcache_insert(u32 ip4, u8 mac[ETH_ALEN])
{
	//fprintf(stderr, "TODO: insert ip->mac entry, and send all the pending packets.\n");
	pthread_mutex_lock(&arpcache.lock);
	int i = 0;
	for (i = 0; i < MAX_ARP_SIZE; i++)
	{
		if(!arpcache.entries[i].valid)//找到空闲的表项，插入
		{
			arpcache.entries[i].ip4 = ip4;
			memcpy(arpcache.entries[i].mac,mac, ETH_ALEN);
			arpcache.entries[i].added = time(NULL);
			arpcache.entries[i].valid = 1;
			break;
		}
	}
	if(i == MAX_ARP_SIZE)//没有空闲表项，随机替换一个
	{
		srand(time(NULL));
		int index = rand() % 32;
		arpcache.entries[index].ip4 =  ip4;
		memcpy(arpcache.entries[index].mac,mac,ETH_ALEN);
		arpcache.entries[index].added = time(NULL);
		arpcache.entries[index].valid = 1;
	}
	struct arp_req *entry,*q;
	list_for_each_entry_safe(entry, q, &arpcache.req_list,list)//查询是否有等待该ip地址的数据包，如果存在，将数据包发送出去
	{
		if(entry->ip4 == ip4)
		{
			struct cached_pkt *pkt_entry,*pkt;
			list_for_each_entry_safe(pkt_entry,pkt,&entry->cached_packets,list)
			{
				struct ether_header *eh = (struct ether_header *)(pkt_entry->packet);
				memcpy(eh->ether_shost, entry->iface->mac, ETH_ALEN);
				memcpy(eh->ether_dhost,mac,ETH_ALEN);
				eh->ether_type = htons(ETH_P_IP);
				iface_send_packet(entry->iface,pkt_entry->packet,pkt_entry->len);
				list_delete_entry(&pkt_entry->list);
				free(pkt_entry);
			}
			list_delete_entry(&entry->list);
			free(entry);
		}
	}
	pthread_mutex_unlock(&arpcache.lock);
}

// sweep arpcache periodically
//
// For the IP->mac entry, if the entry has been in the table for more than 15
// seconds, remove it from the table.
// For the pending packets, if the arp request is sent out 1 second ago, while 
// the reply has not been received, retransmit the arp request. If the arp
// request has been sent 5 times without receiving arp reply, for each
// pending packet, send icmp packet (DEST_HOST_UNREACHABLE), and drop these
// packets.
void *arpcache_sweep(void *arg) 
{
	while (1) {
		sleep(1);
		//fprintf(stderr, "TODO: sweep arpcache periodically: remove old entries, resend arp requests .\n");
		/*生成一个用于存放需要回复消息的包的列表，在访问缓存结束后进行回复，避免对缓存的访问产生死锁*/
		struct cached_pkt *tmp_list = (struct cached_pkt *)malloc(sizeof(struct cached_pkt));
		init_list_head(&tmp_list->list);

		pthread_mutex_lock(&arpcache.lock);//获取互斥锁
		/*如果一个缓存条目在缓存中已存在超过了15秒，将该条目清除*/
		for (int i = 0; i < MAX_ARP_SIZE; i++)
		{
			if(arpcache.entries[i].valid && (time(NULL) - arpcache.entries[i].added) > ARP_ENTRY_TIMEOUT)
				arpcache.entries[i].valid = 0;
		}

		struct arp_req *entry,*q;
		list_for_each_entry_safe(entry, q, &arpcache.req_list,list)
		{
			/*如果一个IP对应的ARP请求发出去已经超过了1秒且请求次数不大于5次，重新发送ARP请求*/
			if( (time(NULL)- entry->sent) > 1 && entry->retries <= 5)
			{
				entry->sent = time(NULL);
				entry->retries ++;
				arp_send_request(entry->iface,entry->ip4);
			}
			/*如果请求次数超过了5次*/
			else if(entry->retries > ARP_REQUEST_MAX_RETRIES)
			{
				struct cached_pkt *pkt_entry,*pkt;
				list_for_each_entry_safe(pkt_entry,pkt,&entry->cached_packets,list)
				{
					/*生成一个需要回复数据包的复制，放入暂存列表中*/
					struct cached_pkt *tmp = (struct cached_pkt *)malloc(sizeof(struct cached_pkt));
					init_list_head(&tmp->list);
					tmp->len = pkt_entry->len;
					tmp->packet = pkt_entry->packet;
					list_add_tail(&tmp->list,&tmp_list->list);
					list_delete_entry(&pkt_entry->list);//删除掉缓存中的表项
					free(pkt_entry);
				}
				list_delete_entry(&entry->list);
				free(entry);
			}
		}
		pthread_mutex_unlock(&arpcache.lock);//放互斥锁
		/*遍历临时链表，对其中的每个条目，发送ICMP消息*/
		struct cached_pkt *pkt_entry,*pkt;
		list_for_each_entry_safe(pkt_entry,pkt,&tmp_list->list,list)
		{
			icmp_send_packet(pkt_entry->packet,pkt_entry->len,ICMP_DEST_UNREACH,ICMP_HOST_UNREACH);
			list_delete_entry(&pkt_entry->list);
			free(pkt_entry);
		}
	}
	return NULL;
}
