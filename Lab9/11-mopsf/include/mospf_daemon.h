#ifndef __MOSPF_DAEMON_H__
#define __MOSPF_DAEMON_H__

#include "base.h"
#include "types.h"
#include "list.h"

void mospf_init();
void mospf_run();
void handle_mospf_packet(iface_info_t *iface, char *packet, int len);
void sending_mospf_lsu();
void init_graph();
int locate_entry(u32 rid);
void Dijkstra(int prev[], int dist[]);
int min_dist(int *dist, int *visited, int n) ;

int router_list[4];
int graph[4][4];//保存链路状态图信息
#endif
