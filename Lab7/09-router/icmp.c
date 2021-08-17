#include "icmp.h"
#include "ip.h"
#include "rtable.h"
#include "arp.h"
#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// send icmp packet
void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code)
{
	//fprintf(stderr, "TODO: malloc and send icmp packet.\n");
	/*定位待处理的数据包的各部分*/
	struct ether_header *in_eh = (struct ether_header*)(in_pkt);
	struct iphdr *in_iph = packet_to_ip_hdr(in_pkt);

	int packet_len;//待发送数据包的长度

	if (type == ICMP_ECHOREPLY && code == ICMP_NET_UNREACH)
	{
		packet_len = len;//与原数据包相同
	}
	else 
	{
		packet_len = ETHER_HDR_SIZE+IP_BASE_HDR_SIZE+ICMP_HDR_SIZE+IP_HDR_SIZE(in_iph) + 8;//需要拷贝收到数据包的IP头部（>= 20字节）和随后的8字节
	}
	/*生成待发送的数据包*/
	char *packet = (char *)malloc(packet_len);
	struct ether_header *eh = (struct ether_header *)(packet);
	struct iphdr *iph = packet_to_ip_hdr(packet);
	struct icmphdr *icmph = (struct icmphdr *)(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);

	eh->ether_type = htons(ETH_P_IP);
	memcpy(eh->ether_dhost, in_eh->ether_dhost, ETH_ALEN);
	memcpy(eh->ether_shost, in_eh->ether_dhost, ETH_ALEN);
	
	u32 saddr = ntohl(in_iph->saddr);
	rt_entry_t *entry = longest_prefix_match(saddr);
	ip_init_hdr(iph, entry->iface->ip,saddr, packet_len-ETHER_HDR_SIZE, 1);

	icmph->type = type;
	icmph->code = code;

	char *rest_1 = (char *)((char *)in_iph + IP_HDR_SIZE(in_iph) + ICMP_HDR_SIZE - 4);//待答复数据包的剩余部分
	char *rest_2 = (char *)((char *)icmph + ICMP_HDR_SIZE - 4);//新数据包的剩余部分
	
	
	if (type == ICMP_ECHOREPLY && code == ICMP_NET_UNREACH)
	{
		memcpy(rest_2, rest_1, len - ETHER_HDR_SIZE - IP_HDR_SIZE(in_iph) - ICMP_HDR_SIZE + 4);
		icmph->checksum = icmp_checksum(icmph, packet_len - ETHER_HDR_SIZE - IP_HDR_SIZE(in_iph));//重新计算checksum值
	}
	else 
	{
		memset(rest_2, 0, 4);//前4字节设置为0
		memcpy(rest_2 + 4, in_iph, IP_HDR_SIZE(in_iph) + 8);//接着拷贝收到数据包的IP头部（>= 20字节）和随后的8字节
		icmph->checksum = icmp_checksum(icmph, IP_HDR_SIZE(in_iph) + 8 + ICMP_HDR_SIZE);//重新计算checksum值
	}
	ip_send_packet(packet, packet_len);
}