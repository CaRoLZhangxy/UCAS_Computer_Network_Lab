#include "mospf_daemon.h"
#include "mospf_proto.h"
#include "mospf_nbr.h"
#include "mospf_database.h"

#include "ip.h"

#include "list.h"
#include "log.h"
#include "rtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

extern ustack_t *instance;

pthread_mutex_t mospf_lock;

int num;

void mospf_init()
{
	pthread_mutex_init(&mospf_lock, NULL);

	instance->area_id = 0;
	// get the ip address of the first interface
	iface_info_t *iface = list_entry(instance->iface_list.next, iface_info_t, list);
	instance->router_id = iface->ip;
	instance->sequence_num = 0;
	instance->lsuint = MOSPF_DEFAULT_LSUINT;

	iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		iface->helloint = MOSPF_DEFAULT_HELLOINT;
		init_list_head(&iface->nbr_list);
	}

	init_mospf_db();
}

void *sending_mospf_hello_thread(void *param);
void *sending_mospf_lsu_thread(void *param);
void *checking_nbr_thread(void *param);
void *checking_database_thread(void *param);
void *generate_rtable_thread(void *param);


void mospf_run()
{
	pthread_t hello, lsu, nbr, db,rtable;
	pthread_create(&hello, NULL, sending_mospf_hello_thread, NULL);
	pthread_create(&lsu, NULL, sending_mospf_lsu_thread, NULL);
	pthread_create(&nbr, NULL, checking_nbr_thread, NULL);
	pthread_create(&db, NULL, checking_database_thread, NULL);
	pthread_create(&rtable, NULL, generate_rtable_thread, NULL);
}

void *sending_mospf_hello_thread(void *param)
{
	while(1)
	{
		pthread_mutex_lock(&mospf_lock);
		iface_info_t * iface = NULL;
		list_for_each_entry (iface, &instance->iface_list, list) 
		{
			int size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE;
			char * packet = (char*)malloc(size * sizeof(char));
			memset(packet, 0, size);
		
			struct ether_header *eh = (struct ether_header *)packet;
			struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
			struct mospf_hdr *mospf_header = (struct mospf_hdr *)(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);
			struct mospf_hello *hello = (struct mospf_hello *)((char *)mospf_header + MOSPF_HDR_SIZE);

			//ether head
			eh->ether_type = htons(ETH_P_IP);
			memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
			eh->ether_dhost[0] = 0x01;
			eh->ether_dhost[2] = 0x5e;
			eh->ether_dhost[5] = 0x05;

			ip_init_hdr(ip_hdr,iface->ip,MOSPF_ALLSPFRouters,IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE,IPPROTO_MOSPF);

			mospf_init_hdr(mospf_header,MOSPF_TYPE_HELLO,MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE,instance->router_id,instance->area_id);

			mospf_init_hello(hello, iface->mask);

			mospf_header->checksum = mospf_checksum(mospf_header);
			iface_send_packet(iface, packet,size);
		}
		pthread_mutex_unlock(&mospf_lock);
		sleep(MOSPF_DEFAULT_HELLOINT);
	}
	return NULL;
}
void sending_mospf_lsu()
{
	iface_info_t * iface = NULL;
	int nbr = 0;
	//计算得到邻居数目
	list_for_each_entry (iface, &instance->iface_list, list) 
	{ 
		if (iface->num_nbr == 0) 
			nbr ++;
		else 
			nbr += iface->num_nbr;	
	}
	
	struct mospf_lsa *lsa_array = (struct mospf_lsa*)malloc(nbr * MOSPF_LSA_SIZE);
	memset(lsa_array,0,nbr * MOSPF_LSA_SIZE);

	int i = 0;

	list_for_each_entry (iface, &instance->iface_list, list) 
	{
		if (iface->num_nbr == 0) 
		{
			lsa_array[i].mask = htonl(iface->mask);
			lsa_array[i].network = htonl(iface->ip & iface->mask);
			lsa_array[i].rid = 0;
			i++;
		} 
		else 
		{
			mospf_nbr_t *ptr = NULL;
			list_for_each_entry (ptr, &iface->nbr_list, list) 
			{
				lsa_array[i].mask = htonl(ptr->nbr_mask);
				lsa_array[i].network = htonl(ptr->nbr_ip & ptr->nbr_mask);
				lsa_array[i].rid = htonl(ptr->nbr_id);
				i++;
			}
		}	
	}

	instance->sequence_num++;

	int size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_LSU_SIZE + nbr * MOSPF_LSA_SIZE;

	iface = NULL;
	list_for_each_entry (iface, &instance->iface_list, list) 
	{ 
		mospf_nbr_t * ptr = NULL;
		list_for_each_entry (ptr, &iface->nbr_list, list) 
		{
			char * packet = (char*)malloc(size);
			memset(packet,0,size);
			struct ether_header *eh = (struct ether_header *)packet;
			struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
			struct mospf_hdr *mospf_header = (struct mospf_hdr *)(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);
			struct mospf_lsu *lsu = (struct mospf_lsu *)((char *)mospf_header + MOSPF_HDR_SIZE);
			struct mospf_lsa *lsa = (struct mospf_lsa *)((char *)lsu + MOSPF_LSU_SIZE);

			eh->ether_type = htons(ETH_P_IP);
			memcpy(eh->ether_shost, iface->mac, ETH_ALEN);

			ip_init_hdr(ip_hdr,iface->ip,ptr->nbr_ip, size - ETHER_HDR_SIZE,IPPROTO_MOSPF);

			mospf_init_hdr(mospf_header,MOSPF_TYPE_LSU,MOSPF_HDR_SIZE + MOSPF_LSU_SIZE + nbr * MOSPF_LSA_SIZE,instance->router_id,instance->area_id);

			mospf_init_lsu(lsu, nbr);
			memcpy(lsa, lsa_array, nbr * MOSPF_LSA_SIZE);

			mospf_header->checksum = mospf_checksum(mospf_header);
			ip_send_packet(packet, size);
		}
	}	
}
void *checking_nbr_thread(void *param)
{
	//fprintf(stdout, "TODO: neighbor list timeout operation.\n");
	while(1)
	{
		iface_info_t *iface = NULL;
        pthread_mutex_lock(&mospf_lock);
		list_for_each_entry (iface, &instance->iface_list, list) 
		{
			mospf_nbr_t * nbr = NULL, *nbr_next;
			list_for_each_entry_safe(nbr, nbr_next, &iface->nbr_list, list) 
			{
				nbr->alive++;
				if (nbr->alive > 3 * iface->helloint) 
				{
					list_delete_entry(&nbr->list);
					free(nbr);
					iface->num_nbr--;
					sending_mospf_lsu();
				}
			}
		}
        pthread_mutex_unlock(&mospf_lock);
		sleep(1);
	}

	return NULL;
}

void *checking_database_thread(void *param)
{
	//fprintf(stdout, "TODO: link state database timeout operation.\n");
	while (1) 
	{
		pthread_mutex_lock(&mospf_lock);
		mospf_db_entry_t *db = NULL, *db_next;
		list_for_each_entry_safe(db, db_next, &mospf_db, list) 
		{
			db->alive++;
			if (db->alive >= MOSPF_DATABASE_TIMEOUT) 
			{
				list_delete_entry(&db->list);
                free(db);
			}
		}
		pthread_mutex_unlock(&mospf_lock);
		sleep(1);
	}
	return NULL;
}

void handle_mospf_hello(iface_info_t *iface, const char *packet, int len)
{
	//fprintf(stdout, "TODO: handle mOSPF Hello message.\n");
	struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
	struct mospf_hdr *mospf_header = (struct mospf_hdr *)(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);
	struct mospf_hello *hello = (struct mospf_hello *)((char *)mospf_header + MOSPF_HDR_SIZE);
	pthread_mutex_lock(&mospf_lock);
	int find = 0;
	mospf_nbr_t *nbr = NULL;
	list_for_each_entry(nbr, &iface->nbr_list, list) 
	{
		if (nbr->nbr_id == ntohl(mospf_header->rid)) 
		{
			find = 1;
			nbr->alive = 0;
			break;
		}
	}
	if (!find) 
	{
		mospf_nbr_t *new_nbr = (mospf_nbr_t*)malloc(sizeof(mospf_nbr_t));
		new_nbr->nbr_id = ntohl(mospf_header->rid);
		new_nbr->nbr_ip = ntohl(ip_hdr->saddr);
		new_nbr->nbr_mask = ntohl(hello->mask);
		new_nbr->alive = 0;
		list_add_tail(&new_nbr->list, &iface->nbr_list);
		iface->num_nbr++;
		sending_mospf_lsu();
	}
	pthread_mutex_unlock(&mospf_lock);
}

void *sending_mospf_lsu_thread(void *param)
{
	//fprintf(stdout, "TODO: send mOSPF LSU message periodically.\n");
	while (1)
	{
		pthread_mutex_lock(&mospf_lock);
		sending_mospf_lsu();
		pthread_mutex_unlock(&mospf_lock);
		sleep(MOSPF_DEFAULT_LSUINT);
	}

	return NULL;
}

void handle_mospf_lsu(iface_info_t *iface, char *packet, int len)
{
	//fprintf(stdout, "TODO: handle mOSPF LSU message.\n");
	struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
	struct mospf_hdr *mospf_header = (struct mospf_hdr *)(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);
	struct mospf_lsu *lsu = (struct mospf_lsu *)((char *)mospf_header + MOSPF_HDR_SIZE);
	struct mospf_lsa *lsa = (struct mospf_lsa *)((char *)lsu + MOSPF_LSU_SIZE);
	
	pthread_mutex_lock(&mospf_lock);
	int find = 0; 
	int updated = 0;

	mospf_db_entry_t * db = NULL;
	list_for_each_entry (db, &mospf_db, list) 
	{
		if (db->rid == ntohl(mospf_header->rid)) 
		{
			find = 1;
			if (db->seq < ntohs(lsu->seq)) 
			{
				updated = 1;
				db->seq = ntohs(lsu->seq);
				db->nadv = ntohl(lsu->nadv);
				db->alive = 0;
				for (int i = 0; i < db->nadv; i++) 
				{
					db->array[i].network = ntohl(lsa[i].network);
					db->array[i].mask = ntohl(lsa[i].mask);	
					db->array[i].rid = ntohl(lsa[i].rid);
				}
			}
		}
	}
	//printf("1\n");
	if (!find) 
	{
		updated = 1;
		mospf_db_entry_t *new_db = (mospf_db_entry_t *)malloc(sizeof(mospf_db_entry_t));
		new_db->rid = ntohl(mospf_header->rid);
		new_db->seq = ntohs(lsu->seq);
		new_db->nadv = ntohl(lsu->nadv);
		new_db->alive = 0;
		new_db->array = (struct mospf_lsa*)malloc(new_db->nadv * MOSPF_LSA_SIZE);

		for (int i = 0; i < new_db->nadv; i ++ ) 
		{
			new_db->array[i].network = ntohl(lsa[i].network);
			new_db->array[i].mask = ntohl(lsa[i].mask);
			new_db->array[i].rid = ntohl(lsa[i].rid);
		}
		list_add_tail(&new_db->list, &mospf_db);
	}

	pthread_mutex_unlock(&mospf_lock);
	//printf("1\n");
	if(updated)
	{
		lsu->ttl -- ;
		if (lsu->ttl > 0) 
		{
			iface_info_t * iface = NULL;
			list_for_each_entry (iface, &instance->iface_list, list) 
			{
				mospf_nbr_t *nbr = NULL;
				list_for_each_entry(nbr, &iface->nbr_list, list) 
				{
					if (nbr->nbr_id != ntohl(mospf_header->rid)) 
					{
						char *forward = (char *)malloc(len);
						struct iphdr * iph = packet_to_ip_hdr(forward);
						struct mospf_hdr * mospfh = (struct mospf_hdr *)(forward + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);

						memcpy(forward, packet, len);
						
						iph->saddr = htonl(iface->ip);
						iph->daddr = htonl(nbr->nbr_ip);

						mospfh->checksum = mospf_checksum(mospfh);
						iph->checksum = ip_checksum(iph);
						
						ip_send_packet(forward, len);
					}
				}
			}
		}
		fprintf(stdout, "%x  ",instance->router_id);
		fprintf(stdout, "MOSPF Database entries:\n");
		list_for_each_entry(db, &mospf_db, list) 
		{
			for (int i = 0; i < db->nadv; i++) 
				fprintf(stdout, IP_FMT"\t"IP_FMT"\t"IP_FMT"\t"IP_FMT"\n", HOST_IP_FMT_STR(db->rid), HOST_IP_FMT_STR(db->array[i].network), HOST_IP_FMT_STR(db->array[i].mask), HOST_IP_FMT_STR(db->array[i].rid));
		}

	}
}

void handle_mospf_packet(iface_info_t *iface, char *packet, int len)
{
	struct iphdr *ip = (struct iphdr *)(packet + ETHER_HDR_SIZE);
	struct mospf_hdr *mospf = (struct mospf_hdr *)((char *)ip + IP_HDR_SIZE(ip));

	if (mospf->version != MOSPF_VERSION) {
		log(ERROR, "received mospf packet with incorrect version (%d)", mospf->version);
		return ;
	}
	if (mospf->checksum != mospf_checksum(mospf)) {
		log(ERROR, "received mospf packet with incorrect checksum");
		return ;
	}
	if (ntohl(mospf->aid) != instance->area_id) {
		log(ERROR, "received mospf packet with incorrect area id");
		return ;
	}

	switch (mospf->type) {
		case MOSPF_TYPE_HELLO:
			handle_mospf_hello(iface, packet, len);
			break;
		case MOSPF_TYPE_LSU:
			handle_mospf_lsu(iface, packet, len);
			break;
		default:
			log(ERROR, "received mospf packet with unknown type (%d).", mospf->type);
			break;
	}
}

int locate_entry(u32 rid) 
{
	for(int i = 0; i < num; i ++ )
		if(router_list[i] == rid) 
            return i;
	return -1;
}

void init_graph()
{
	memset(graph, INT8_MAX -1, sizeof(graph));
    mospf_db_entry_t *db = NULL;
	router_list[0] = instance->router_id;
	num = 1;

	list_for_each_entry(db, &mospf_db, list) 
	{
		router_list[num] = db->rid;
		num++;
	}
    
    db = NULL;
	//printf("%d\n", num);
	list_for_each_entry(db, &mospf_db, list) 
	{
		int u = locate_entry(db->rid);
		for(int i = 0; i < db->nadv; i ++ ) 
		{
			if(db->array[i].rid)
			{
				int v = locate_entry(db->array[i].rid);
				graph[u][v] = graph[v][u] = 1;
			}
		}
	}
}
int min_dist(int *dist, int *visited, int n) 
{
	int i = -1;
	for (int u = 0; u < n; u++) 
	{
		if(!visited[u])
		{
			if (i == -1 || dist[u] < dist[i]) {
				i = u;
			}
		}
	}
	return i;
}
void Dijkstra(int prev[], int dist[]) 
{
    int visited[4];
	for(int i = 0; i < 4; i++) 
	{
		dist[i] = INT8_MAX;
		prev[i] = -1;
		visited[i] = 0;
	}

	dist[0] = 0;

	for(int i = 0; i < num; i++) 
	{
		int u = min_dist(dist, visited,num);
		visited[u] = 1;
		for (int v = 0; v < num; v++)
		{
			if (visited[v] == 0 && dist[u] + graph[u][v] < dist[v] && graph[u][v] > 0) 
			{
				dist[v] = dist[u] + graph[u][v];
				prev[v] = u;
			}
		}
	}
}
void *generate_rtable_thread(void *param)
{
	while(1) {
		sleep(1);
		int prev[4];
    	int dist[4];
		init_graph();
		Dijkstra(prev, dist);
		int visited[4] = {0};
		visited[0] = 1; 
		
		rt_entry_t *rt_entry, *rt_entry_next;
		list_for_each_entry_safe (rt_entry, rt_entry_next, &rtable, list) 
		{
			if(rt_entry->gw) 
				remove_rt_entry(rt_entry);
		}
		for (int i = 0; i < num; i ++ ) 
		{
			int t = -1;
			for(int j = 0; j < num; j ++ ) 
			{
				if(!visited[j])
				{
					if(t == -1 || dist[j] < dist[t])
						t = j;
				}
			}
			visited[t] = 1;
			mospf_db_entry_t *db;
			list_for_each_entry(db, &mospf_db, list) 
			{
				if(db->rid == router_list[t]) 
				{
					int next_hop;
					while(prev[t] != 0) 
						t = prev[t];
					next_hop = t;
					iface_info_t *iface;
					u32 gw;
					int find = 0;

					list_for_each_entry (iface, &instance->iface_list, list) 
					{
						mospf_nbr_t *nbr;
						list_for_each_entry (nbr, &iface->nbr_list, list) 
						{
							if(nbr->nbr_id == router_list[next_hop]) 
							{
								find = 1;
								gw = nbr->nbr_ip;
								break;
							}
						}
						if(find)
							break;
					}
					if(!find)
						break;
					for(int i = 0; i < db->nadv; i ++) 
					{
						rt_entry_t *entry = NULL;
						find = 0;
						list_for_each_entry(entry, &rtable, list) 
						{
							if(entry->dest == db->array[i].network && entry->mask == db->array[i].mask) 
							{
								find = 1;
								break;
							}
						}
						if(!find) 
						{
							rt_entry_t *new = new_rt_entry(db->array[i].network, db->array[i].mask, gw, iface);
							add_rt_entry(new);
						}
					}
				}
			}
		}
		print_rtable();
	}
}