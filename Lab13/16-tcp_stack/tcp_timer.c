#include "tcp.h"
#include "tcp_timer.h"
#include "tcp_sock.h"

#include <stdio.h>
#include <unistd.h>

static struct list_head timer_list;
pthread_mutex_t timer_lock;

// scan the timer_list, find the tcp sock which stays for at 2*MSL, release it
void tcp_scan_timer_list()
{
	// TODO: implement %s please.\n, __FUNCTION__
	struct tcp_timer *timer, *q;
	struct tcp_sock *tsk;
    pthread_mutex_lock(&timer_lock);
	list_for_each_entry_safe(timer, q, &timer_list, list) 
	{
	    if (timer->type == 0) 
		{
            timer->timeout -= TCP_TIMER_SCAN_INTERVAL;
            if (timer->timeout <= 0) 
			{
                list_delete_entry(&timer->list);
				tsk = timewait_to_tcp_sock(timer);
				if (!tsk->parent)
					tcp_bind_unhash(tsk);
				tcp_set_state(tsk, TCP_CLOSED);
				free_tcp_sock(tsk);
            }
        } 
		else 
		{
	        tsk = retranstimer_to_tcp_sock(timer);
	        struct send_buffer *entry;
	        list_for_each_entry(entry, &tsk->send_buf, list) 
			{
                entry->timeout -= TCP_TIMER_SCAN_INTERVAL;
	            if (!entry->timeout) 
				{
	                if (entry->times++ == 3) 
					{
	                    entry->times = 1;
	                    entry->timeout = TCP_RETRANS_INTERVAL_INITIAL;
	                } 
					else 
					{
	                    char *temp = (char*)malloc(entry->len * sizeof(char));
	                    memcpy(temp, entry->packet, entry->len);
	                    ip_send_packet(temp, entry->len);
	                    if (entry->times == 2) 
							entry->timeout = entry->times * TCP_RETRANS_INTERVAL_INITIAL;
	                    else 
							entry->timeout = 4 * TCP_RETRANS_INTERVAL_INITIAL;
	                }
	            }
	        }
	    }
	}
    pthread_mutex_unlock(&timer_lock);
}

// set the timewait timer of a tcp sock, by adding the timer into timer_list
void tcp_set_timewait_timer(struct tcp_sock *tsk)
{
	// TODO: implement %s please.\n, __FUNCTION__
	tsk->timewait.type = 0;
	tsk->timewait.timeout = TCP_TIMEWAIT_TIMEOUT;
	list_add_tail(&tsk->timewait.list, &timer_list);
	tsk->ref_cnt++;
}

void tcp_set_retrans_timer(struct tcp_sock *tsk) {
    tsk->retrans_timer.type = 1;
    tsk->retrans_timer.timeout = TCP_RETRANS_INTERVAL_INITIAL;
    pthread_mutex_lock(&timer_lock);
    struct tcp_timer *timer;
    list_for_each_entry(timer, &timer_list, list) 
	{
        if (retranstimer_to_tcp_sock(timer) == tsk) 
		{
            pthread_mutex_unlock(&timer_lock);
            return;
        }
    }
    list_add_tail(&tsk->retrans_timer.list, &timer_list);
    pthread_mutex_unlock(&timer_lock);
    tsk->ref_cnt++;
}

void tcp_unset_retrans_timer(struct tcp_sock *tsk) 
{
    list_delete_entry(&tsk->retrans_timer.list);
    tsk->ref_cnt--;
}

// scan the timer_list periodically by calling tcp_scan_timer_list
void *tcp_timer_thread(void *arg)
{
	init_list_head(&timer_list);
	pthread_mutex_init(&timer_lock, NULL);
	while (1) {
		usleep(TCP_TIMER_SCAN_INTERVAL);
		tcp_scan_timer_list();
	}

	return NULL;
}