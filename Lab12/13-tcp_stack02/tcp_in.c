#include "tcp.h"
#include "tcp_sock.h"
#include "tcp_timer.h"

#include "log.h"
#include "ring_buffer.h"

#include <stdlib.h>
// update the snd_wnd of tcp_sock
//
// if the snd_wnd before updating is zero, notify tcp_sock_send (wait_send)
static inline void tcp_update_window(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u16 old_snd_wnd = tsk->snd_wnd;
	tsk->snd_wnd = cb->rwnd;
	if (old_snd_wnd == 0)
		wake_up(tsk->wait_send);
}

// update the snd_wnd safely: cb->ack should be between snd_una and snd_nxt
static inline void tcp_update_window_safe(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	if (less_or_equal_32b(tsk->snd_una, cb->ack) && less_or_equal_32b(cb->ack, tsk->snd_nxt))
		tcp_update_window(tsk, cb);
}

#ifndef max
#	define max(x,y) ((x)>(y) ? (x) : (y))
#endif

// check whether the sequence number of the incoming packet is in the receiving
// window
static inline int is_tcp_seq_valid(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u32 rcv_end = tsk->rcv_nxt + max(tsk->rcv_wnd, 1);
	if (less_than_32b(cb->seq, rcv_end) && less_or_equal_32b(tsk->rcv_nxt, cb->seq_end)) {
		return 1;
	}
	else {
		log(ERROR, "received packet with invalid seq, drop it.");
		return 0;
	}
}

// Process the incoming packet according to TCP state machine. 
void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	//fprintf(stdout, "TODO: implement %s please.\n", __FUNCTION__);

	if(cb->flags & TCP_RST)
	{
		tcp_sock_close(tsk);
		free(tsk);
		return;
	}
	switch(tsk->state)
	{
		case TCP_CLOSED:
			tcp_send_reset(cb);
			break;
		case TCP_LISTEN:
			if(cb->flags & TCP_SYN)
			{
				struct tcp_sock *child_sk = alloc_tcp_sock();
				child_sk->parent = tsk;
				child_sk->sk_sip = cb->daddr;
				child_sk->sk_sport = cb->dport;
				child_sk->sk_dip = cb->saddr;
				child_sk->sk_dport = cb->sport;
				child_sk->iss = tcp_new_iss();
				child_sk->rcv_nxt = cb->seq + 1;
				child_sk->snd_nxt = child_sk->iss;
				
				list_add_tail(&child_sk->list, &tsk->listen_queue);
				tcp_send_control_packet(child_sk, TCP_ACK | TCP_SYN);
				tcp_set_state(child_sk, TCP_SYN_RECV);
				tcp_hash(child_sk);
			}
			else
				tcp_send_reset(cb);
			break;
		case TCP_SYN_SENT:
			if(cb->flags & (TCP_SYN | TCP_ACK)) 
			{
				tsk->rcv_nxt = cb->seq + 1;
				tsk->snd_una = cb->ack;
				tcp_send_control_packet(tsk, TCP_ACK);
				tcp_set_state(tsk, TCP_ESTABLISHED);
				wake_up(tsk->wait_connect);
			}
			else
				tcp_send_reset(cb);
			break;
		case TCP_SYN_RECV:
			if(cb->flags & TCP_ACK) 
			{
				tcp_sock_accept_enqueue(tsk);
				tsk->rcv_nxt = cb->seq;
		    	tsk->snd_una = cb->ack;
				wake_up(tsk->parent->wait_accept);
			}
			else
				tcp_send_reset(cb);
			break;
		case TCP_ESTABLISHED:
			if(cb->flags & TCP_FIN)
			{
				tsk->rcv_nxt = cb->seq+1;
				wait_exit(tsk->wait_recv);
				tcp_set_state(tsk, TCP_CLOSE_WAIT);
				tcp_send_control_packet(tsk, TCP_ACK);	
			}
			else if(cb->flags == (TCP_PSH | TCP_ACK))
			{
				if(cb->pl_len == 0)
				{
					tsk->snd_una = cb->ack;
					tsk->rcv_nxt = cb->seq + 1;
					tcp_update_window_safe(tsk, cb);
				}
				else
				{
					while (ring_buffer_full(tsk->rcv_buf)) 
						sleep_on(tsk->wait_recv);
					pthread_mutex_lock(&tsk->rcv_buf->lock);
					write_ring_buffer(tsk->rcv_buf, cb->payload, cb->pl_len);
					pthread_mutex_unlock(&tsk->rcv_buf->lock);
					tsk->rcv_nxt = cb->seq + cb->pl_len;
					tsk->snd_una = cb->ack;
					wake_up(tsk->wait_recv);
					tcp_send_control_packet(tsk, TCP_ACK);
				}
			}
			break;
		case TCP_FIN_WAIT_1:
			if(cb->flags & TCP_ACK)
				tcp_set_state(tsk, TCP_FIN_WAIT_2);	
			break;
		case TCP_FIN_WAIT_2:
			if(cb->flags & TCP_FIN)
			{
				tsk->rcv_nxt = cb->seq + 1;
				tcp_send_control_packet(tsk, TCP_ACK);	
				tcp_set_state(tsk, TCP_TIME_WAIT);
				tcp_set_timewait_timer(tsk);		
			}
			break;
		case TCP_LAST_ACK:
			if(cb->flags & TCP_ACK)
				tcp_set_state(tsk, TCP_CLOSED);	
			break;
		default:
			break;
	}
}
