#include "ip.h"
#include "icmp.h"
#include "rtable.h"
#include "arp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// handle ip packet
//
// If the packet is ICMP echo request and the destination IP address is equal to
// the IP address of the iface, send ICMP echo reply; otherwise, forward the
// packet.
void handle_ip_packet(iface_info_t *iface, char *packet, int len)
{
	//fprintf(stderr, "TODO: handle ip packet.\n");
	/*定位待处理数据包的各部分*/
	struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
	struct icmphdr *icmp_hdr = (struct icmphdr *)IP_DATA(ip_hdr);
	struct ether_header *eh = (struct ether_header *)(packet);


	u32 dst = htonl(ip_hdr->daddr);
	memcpy(eh->ether_shost,iface->mac, ETH_ALEN);

	/*如果收到的包目的是本路由器端口,并且 ICMP 首部 type为 请求 ,回应 ICMP 报文*/
	if(icmp_hdr->type == ICMP_ECHOREQUEST && iface->ip == dst)//
	{
		icmp_send_packet(packet,len,ICMP_ECHOREPLY,ICMP_NET_UNREACH);
	}
	else//转发报文
	{
		/*对IP头部的TTL值进行减一操作，如果该值 <= 0，则将该数据包丢弃，并回复ICMP信息*/
		ip_hdr->ttl --;
		if(ip_hdr->ttl <= 0)
		{
			icmp_send_packet(packet,len,ICMP_TIME_EXCEEDED,ICMP_NET_UNREACH);
			free(packet);
			return;
		}
		/*IP头部数据已经发生变化，需要重新设置checksum*/
		ip_hdr->checksum = ip_checksum(ip_hdr);
		/*发送新的数据包*/
		rt_entry_t *entry = longest_prefix_match(dst);
		if(entry)
		{
			u32 dest = entry->gw ? entry->gw : dst;
			iface_send_packet_by_arp(entry->iface,dest,packet, len);
		}
		else
		{
			icmp_send_packet(packet,len,ICMP_DEST_UNREACH,ICMP_NET_UNREACH);
		}
	}
}
