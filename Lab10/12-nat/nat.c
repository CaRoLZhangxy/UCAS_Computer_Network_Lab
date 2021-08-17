#include "nat.h"
#include "ip.h"
#include "icmp.h"
#include "tcp.h"
#include "rtable.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static struct nat_table nat;

// get the interface from iface name
static iface_info_t *if_name_to_iface(const char *if_name)
{
	iface_info_t *iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		if (strcmp(iface->name, if_name) == 0)
			return iface;
	}

	log(ERROR, "Could not find the desired interface according to if_name '%s'", if_name);
	return NULL;
}

// determine the direction of the packet, DIR_IN / DIR_OUT / DIR_INVALID
static int get_packet_direction(char *packet)
{
	fprintf(stdout, "TODO: determine the direction of this packet.\n");
	struct iphdr *iph = packet_to_ip_hdr(packet);
	rt_entry_t *entry = longest_prefix_match(ntohl(iph->saddr));
	if(entry->iface->index == nat.internal_iface->index)
		return DIR_OUT;
	else if(entry->iface->index == nat.external_iface->index)
		return DIR_IN;
	return DIR_INVALID;
}
int alloc_port() {
	for (int i = NAT_PORT_MIN; i < NAT_PORT_MAX; i++) 
	{
		if (!nat.assigned_ports[i]) 
		{
			nat.assigned_ports[i] = 1;
			return i;
		}
	}
	return 0;
}
// do translation for the packet: replace the ip/port, recalculate ip & tcp
// checksum, update the statistics of the tcp connection
void do_translation(iface_info_t *iface, char *packet, int len, int dir)
{
	
	fprintf(stdout, "TODO: do translation for this packet.\n");
	pthread_mutex_lock(&nat.lock);
	struct iphdr *iph = packet_to_ip_hdr(packet);
	struct tcphdr *tcph = packet_to_tcp_hdr(packet);

	u32 rmt_ip = dir == DIR_IN ? ntohl(iph->saddr) : ntohl(iph->daddr);
	u16 rmt_port = dir == DIR_IN ? ntohs(tcph->sport) : ntohs(tcph->dport);

	char tmp[6];
	memcpy(tmp,(char *)&rmt_ip,4);
	memcpy(tmp + 4,(char *)&rmt_port,2);
	u8 index = hash8(tmp,6);

	struct nat_mapping *entry = NULL;
	if(dir == DIR_IN) 
	{
		int find = 0;
		list_for_each_entry(entry,&nat.nat_mapping_list[index],list)
		{
			if(entry->external_ip == ntohl(iph->daddr) && entry->external_port == ntohs(tcph->dport))
			{
				find = 1;
				break;
			}
		}
		if(!find)
		{
			int rfind = 0;
			struct dnat_rule *rule_entry = NULL;
			list_for_each_entry(rule_entry, &nat.rules, list)
			{
				if(ntohl(iph->daddr) == rule_entry->external_ip && ntohs(tcph->dport) == rule_entry->external_port)
				{
					rfind = 1;
					break;
				}
			}
			if(rfind)
			{
				entry = (struct nat_mapping *)malloc(sizeof(struct nat_mapping));
				memset(entry,0, sizeof(struct nat_mapping));
				entry->internal_ip = rule_entry->internal_ip;
				entry->internal_port = rule_entry->internal_port;
				entry->external_ip = rule_entry->external_ip;
				entry->external_port = rule_entry->external_port;
				entry->remote_ip = rmt_ip;
				entry->remote_port = rmt_port;
				entry->update_time = time(NULL);
				list_add_tail(&entry->list, &nat.nat_mapping_list[index]);
			}
			else
			{
				pthread_mutex_unlock(&nat.lock);
				return NULL;
			}
		}
		tcph->dport = htons(entry->internal_port);
		iph->daddr = htonl(entry->internal_ip);
		entry->conn.external_fin = (tcph->flags == TCP_FIN) ? TCP_FIN : 0;
		if(ntohl(tcph->seq) > entry->conn.external_seq_end)
			entry->conn.external_seq_end = ntohl(tcph->seq);
		if(ntohl(tcph->ack) > entry->conn.external_ack && tcph->flags == TCP_ACK)
			entry->conn.external_ack = ntohl(tcph->ack);
	}
	else if (dir == DIR_OUT)
	{
		int find = 0;
		list_for_each_entry(entry,&nat.nat_mapping_list[index],list)
		{
			if(entry->internal_ip == ntohl(iph->saddr) && entry->internal_port == ntohs(tcph->sport))
			{
				find = 1;
				break;
			}
		}
		if(!find)
		{
			entry = (struct nat_mapping*)malloc(sizeof(struct nat_mapping));
			memset(entry,0, sizeof(struct nat_mapping));
			entry->internal_ip = ntohl(iph->saddr);
			entry->internal_port = ntohs(tcph->sport);
			entry->external_ip = nat.external_iface->ip;
			entry->external_port = alloc_port();
			entry->remote_ip = rmt_ip;
			entry->remote_port = rmt_port;
			entry->update_time = time(NULL);
			list_add_tail(&entry->list, &nat.nat_mapping_list[index]);
		}
		tcph->sport = htons(entry->external_port);
		iph->saddr = htonl(entry->external_ip);
		if(tcph->flags == TCP_FIN)
			entry->conn.internal_fin = TCP_FIN;
		if(ntohl(tcph->seq) > entry->conn.internal_seq_end)
			entry->conn.internal_seq_end = ntohl(tcph->seq);
		if(ntohl(tcph->ack) > entry->conn.internal_ack && tcph->flags == TCP_ACK)
			entry->conn.internal_ack = ntohl(tcph->ack);
	}
	tcph->checksum = tcp_checksum(iph, tcph);
	iph->checksum = ip_checksum(iph);
	ip_send_packet(packet, len);
	pthread_mutex_unlock(&nat.lock);
	
}

void nat_translate_packet(iface_info_t *iface, char *packet, int len)
{
	int dir = get_packet_direction(packet);
	if (dir == DIR_INVALID) {
		log(ERROR, "invalid packet direction, drop it.");
		icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
		free(packet);
		return ;
	}

	struct iphdr *ip = packet_to_ip_hdr(packet);
	if (ip->protocol != IPPROTO_TCP) {
		log(ERROR, "received non-TCP packet (0x%0hhx), drop it", ip->protocol);
		free(packet);
		return ;
	}

	do_translation(iface, packet, len, dir);
}

// check whether the flow is finished according to FIN bit and sequence number
// XXX: seq_end is calculated by `tcp_seq_end` in tcp.h
static int is_flow_finished(struct nat_connection *conn)
{
    return (conn->internal_fin && conn->external_fin) && \
            (conn->internal_ack >= conn->external_seq_end) && \
            (conn->external_ack >= conn->internal_seq_end);
}

// nat timeout thread: find the finished flows, remove them and free port
// resource
void *nat_timeout()
{
	while (1) {
		fprintf(stdout, "TODO: sweep finished flows periodically.\n");
		pthread_mutex_lock(&nat.lock);
		for (int i = 0; i < HASH_8BITS; i++) 
		{
			struct nat_mapping * entry, *q;
			list_for_each_entry_safe (entry, q, &nat.nat_mapping_list[i], list) 
			{
				if (time(NULL) - entry->update_time > TCP_ESTABLISHED_TIMEOUT) 
				{
					nat.assigned_ports[entry->external_port] = 0;
					list_delete_entry(&entry->list);
					free(entry);
				}
				else if (is_flow_finished(&entry->conn)) 
				{
					nat.assigned_ports[entry->external_port] = 0;
					list_delete_entry(&entry->list);
					free(entry);
				}
			}
		}
		pthread_mutex_unlock(&nat.lock);
		sleep(1);
	}

	return NULL;
}
u32 ip_trans(const char *str_ip)
{
    u32 ip = 0;
    unsigned char tmp = 0;
    while (1) 
	{
        if (*str_ip != '\0' && *str_ip != '.') 
            tmp = tmp * 10 + *str_ip - '0';
        else 
        {
            ip = (ip << 8) + tmp;
            if (*str_ip == '\0')
                break;
            tmp = 0;
        }
        str_ip++;
    }
    return ip;
}
int parse_config(const char *filename)
{
	fprintf(stdout, "TODO: parse config file, including i-iface, e-iface (and dnat-rules if existing).\n");
	FILE *fd; 			
					
	if((fd = fopen(filename, "r")) == NULL) 
		return -1; 

	char line[100];
	while (fgets(line, 100, fd) != NULL)
	{
		if (strstr(line, "internal-iface")) 
		{
			char *internal_iface;
			char *iface_name;
			internal_iface = strtok(line,": ");
			iface_name = strtok(NULL,"\n");
			iface_name ++;
			printf("%s\n",iface_name);
			iface_info_t * iface = if_name_to_iface(iface_name);
			nat.internal_iface = iface;
		} 
		else if (strstr(line, "external-iface")) 
		{
			char *external_iface;
			char *iface_name;
			external_iface = strtok(line,": ");
			iface_name = strtok(NULL,"\n");
			iface_name++;
			printf("%s\n",iface_name);
			iface_info_t * iface = if_name_to_iface(iface_name);
			nat.external_iface = iface;
		} 
		else if (strstr(line, "dnat-rules")) 
		{
			char *dnat = strtok(line, ": ");
			char *net1 = strtok(NULL, "-> ");
			char *net2 = strtok(NULL, "-> ");
			char *ip1 = strtok(net1, ":");
			char *port1 = strtok(NULL, ":");
			char *ip2 = strtok(net2, ":");
			char *port2 = strtok(NULL, ":");

			u16 external_port = atoi(port1);
			u16 internal_port = atoi(port2);
			u32 external_ip = ip_trans(ip1);
			u32 internal_ip = ip_trans(ip2);


			struct nat_mapping * entry = (struct nat_mapping *)malloc(sizeof(struct nat_mapping));
			memset(entry,0,sizeof(struct nat_mapping));
			entry->external_ip = external_ip;
			entry->internal_ip = internal_ip;
			entry->external_port = external_port;
			entry->internal_port = internal_port;
			entry->update_time = time(NULL);

			struct dnat_rule * rule = (struct dnat_rule *)malloc(sizeof(struct dnat_rule));
			rule->external_ip = external_ip;
			rule->internal_ip = internal_ip;
			rule->external_port = external_port;
			rule->internal_port = internal_port;
			list_add_tail(&rule->list ,&nat.rules);
		}
	}
	return 0;
}

// initialize
void nat_init(const char *config_file)
{
	memset(&nat, 0, sizeof(nat));

	for (int i = 0; i < HASH_8BITS; i++)
		init_list_head(&nat.nat_mapping_list[i]);

	init_list_head(&nat.rules);

	// seems unnecessary
	memset(nat.assigned_ports, 0, sizeof(nat.assigned_ports));

	parse_config(config_file);
	pthread_mutex_init(&nat.lock, NULL);

	pthread_create(&nat.thread, NULL, nat_timeout, NULL);
}

void nat_exit()
{
	fprintf(stdout, "TODO: release all resources allocated.\n");
	pthread_mutex_lock(&nat.lock);

	for (u8 i = 0; i < HASH_8BITS; i++) 
	{
		struct nat_mapping *entry, *q;
		list_for_each_entry_safe(entry, q, &nat.nat_mapping_list[i], list) 
		{
			list_delete_entry(&entry->list);
			free(entry);
		}
	}

	pthread_mutex_unlock(&nat.lock);
}
