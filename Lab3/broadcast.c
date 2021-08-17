#include "base.h"
#include <stdio.h>

extern ustack_t *instance;

void broadcast_packet(iface_info_t *iface, const char *packet, int len)
{
	// TODO: broadcast packet 
	fprintf(stdout, "TODO: broadcast packet.\n");
	//struct list_head list = instance -> iface_list;
	iface_info_t *entry;
	list_for_each_entry(entry,&instance -> iface_list,list)
	{
		if(entry->index != iface->index)
			iface_send_packet(entry,packet,len);
	}
}
