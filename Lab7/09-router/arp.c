#include "arp.h"
#include "base.h"
#include "types.h"
#include "ether.h"
#include "arpcache.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "log.h"

// send an arp request: encapsulate an arp request packet, send it out through
// iface_send_packet
void arp_send_request(iface_info_t *iface, u32 dst_ip)
{
	//fprintf(stderr, "TODO: send arp request when lookup failed in arpcache.\n");
	/*生成要发送的包，并定位各部分内容*/
	char *packet = (char *)malloc(sizeof(struct ether_header) + sizeof(struct ether_arp));
	struct ether_header *header = (struct ether_header *)packet;
	struct ether_arp *arp = (struct ether_arp *)(packet + sizeof(struct ether_header));
	/*填充etherhead的内容*/
	header->ether_type = htons(ETH_P_ARP);
	memcpy(header->ether_shost,iface->mac, ETH_ALEN);
	memset(header->ether_dhost,0xff, ETH_ALEN);//广播包
	/*填充arp协议的内容*/
	arp->arp_hrd = htons(ARPHRD_ETHER);
	arp->arp_pro = htons(0x0800);
	arp->arp_hln = 6;
	arp->arp_pln = 4;
	arp->arp_op =  htons(ARPOP_REQUEST);//代表类型为arp请求
	memcpy(arp->arp_sha,iface->mac,ETH_ALEN);
	memset(arp->arp_tha,0, ETH_ALEN);//当为ARP请求时，Target HW Addr置空
	arp->arp_spa = htonl(iface->ip);
	arp->arp_tpa = htonl(dst_ip);
	/*发送包*/
	iface_send_packet(iface,packet,sizeof(struct ether_header) + sizeof(struct ether_arp));
}

// send an arp reply packet: encapsulate an arp reply packet, send it out
// through iface_send_packet
void arp_send_reply(iface_info_t *iface, struct ether_arp *req_hdr)
{
	//fprintf(stderr, "TODO: send arp reply when receiving arp request.\n");
	/*生成要发送的包，并定位各部分内容*/
	char *packet = (char *)malloc(sizeof(struct ether_header) + sizeof(struct ether_arp));
	struct ether_header *header = (struct ether_header *)packet;
	struct ether_arp *arp = (struct ether_arp *)(packet + sizeof(struct ether_header));
	/*填充etherhead的内容*/
	header->ether_type = htons(ETH_P_ARP);
	memcpy(header->ether_shost,iface->mac, ETH_ALEN);
	memcpy(header->ether_dhost,req_hdr->arp_sha, ETH_ALEN);
	/*填充arp协议的内容*/
	arp->arp_hrd = htons(ARPHRD_ETHER);
	arp->arp_pro = htons(0x0800);
	arp->arp_hln = 6;
	arp->arp_pln = 4;
	arp->arp_op =  htons(ARPOP_REPLY);//代表类型为arp回复
	memcpy(arp->arp_sha,iface->mac,ETH_ALEN);
	memcpy(arp->arp_tha,req_hdr->arp_sha, ETH_ALEN);
	arp->arp_spa = htonl(iface->ip);
	arp->arp_tpa = req_hdr->arp_spa;
	/*发送包*/
	iface_send_packet(iface,packet,sizeof(struct ether_header) + sizeof(struct ether_arp));
}

void handle_arp_packet(iface_info_t *iface, char *packet, int len)
{
	//fprintf(stderr, "TODO: process arp packet: arp request & arp reply.\n");
	struct ether_arp *arp = (struct ether_arp *)(packet + sizeof(struct ether_header));
	
	switch(ntohs(arp->arp_op))
	{
		case ARPOP_REQUEST:
			if(ntohl(arp->arp_tpa) == iface->ip)
				arp_send_reply(iface, arp);
			arpcache_insert(htonl(arp->arp_spa),arp->arp_sha);
			break;
		case ARPOP_REPLY:
			arpcache_insert(htonl(arp->arp_spa),arp->arp_sha);
			break;
		default:
			free(packet);
			break;
	}
	
}

// send (IP) packet through arpcache lookup 
//
// Lookup the mac address of dst_ip in arpcache. If it is found, fill the
// ethernet header and emit the packet by iface_send_packet, otherwise, pending 
// this packet into arpcache, and send arp request.
void iface_send_packet_by_arp(iface_info_t *iface, u32 dst_ip, char *packet, int len)
{
	struct ether_header *eh = (struct ether_header *)packet;
	memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
	eh->ether_type = htons(ETH_P_IP);

	u8 dst_mac[ETH_ALEN];
	int found = arpcache_lookup(dst_ip, dst_mac);
	if (found) {
		//log(DEBUG, "found the mac of %x, send this packet", dst_ip);
		memcpy(eh->ether_dhost, dst_mac, ETH_ALEN);
		iface_send_packet(iface, packet, len);
	}
	else {
		//log(DEBUG, "lookup %x failed, pend this packet", dst_ip);
		arpcache_append_packet(iface, dst_ip, packet, len);
	}
}
