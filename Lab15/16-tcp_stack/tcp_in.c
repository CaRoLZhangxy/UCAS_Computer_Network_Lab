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
	tsk->adv_wnd = cb->rwnd;
    tsk->snd_wnd = min(tsk->adv_wnd, tsk->cwnd * (MSS));
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
	printf("state,flags,len:%d,%x,%d\n",tsk->state,cb->flags,cb->pl_len);
    if (((cb->flags) & TCP_PSH) == 0)
		tsk->rcv_nxt = cb->seq_end;
    if ((cb->flags) == TCP_ACK) 
	{
        if (cb->ack == tsk->snd_una) 
        {
            tsk->ack_time++;
            switch (tsk->nrstate) 
            {
                case OPEN:
                    if (tsk->ack_time > 2) 
                    {
                        tsk->nrstate = RECOVER;
                        tsk->ssthresh = (tsk->cwnd + 1) / 2;
                        tsk->recovery_point = tsk->snd_nxt;
                        struct send_buffer *entry = list_entry((&tsk->send_buf)->next,typeof(*entry), list);
                        if (!list_empty(&tsk->send_buf)) 
                        {
                            
                            char *temp = (char *) malloc(entry->len * sizeof(char));
                            memcpy(temp, entry->packet, entry->len);
                            ip_send_packet(temp, entry->len);
                        }
                    }
                    break;
                case RECOVER:
                    if (tsk->ack_time > 1) 
                    {
                        tsk->ack_time -= 2;
                        tsk->cwnd -= 1;
                        if (tsk->cwnd < 1) 
                            tsk->cwnd = 1;
                    }
                    break;
                default:
                    break;
            }
        }
        else
        {
            tsk->ack_time = 0;
            struct send_buffer *entry, *q;
            list_for_each_entry_safe(entry, q, &tsk->send_buf, list) 
            {
                if (entry->seq_end > cb->ack) 
                    break;
                else 
                {
                    tsk->snd_una = entry->seq_end;
                    tcp_sndbuf_pop(tsk, entry);
                    if(tsk->nrstate == OPEN)
                    {
                        if (tsk->cwnd < tsk->ssthresh) 
                            tsk->cwnd ++;
                        else 
                        {
                            tsk->cnt ++;
                            if (tsk->cnt >= tsk->cwnd) 
                            {
                                tsk->cnt = 0;
                                tsk->cwnd ++;
                            }
                        }
                    }
                }
            }
            switch (tsk->nrstate) 
            {
                case RECOVER:
                    if (tsk->recovery_point <= cb->ack) 
                        tsk->nrstate = OPEN;
                    else 
                    {
                        char *temp = (char *) malloc(entry->len * sizeof(char));
                        memcpy(temp, entry->packet, entry->len);
                        ip_send_packet(temp, entry->len);
                    }
                    break;
                case LOSS:
                    if (tsk->recovery_point <= cb->ack) 
                        tsk->nrstate = OPEN;
                    break;
                default:
                    break;
            }
        }
        //pthread_mutex_lock(&tsk->file_lock);
        //cwnd_dump(tsk);
        //pthread_mutex_unlock(&tsk->file_lock);
        tcp_update_window_safe(tsk, cb);
    }
    switch (cb->flags) 
	{
        case TCP_SYN:
            switch (tsk->state) 
			{
                case TCP_LISTEN:
					;
                    printf("1\n");
                    struct tcp_sock *child_sk = alloc_tcp_sock();
                    list_add_tail(&child_sk->list, &tsk->listen_queue);
                    child_sk->sk_sip = cb->daddr;
                    child_sk->sk_dip = cb->saddr;
                    child_sk->sk_dport = cb->sport;
                    child_sk->sk_sport = cb->dport;
                    child_sk->parent = tsk;
                    child_sk->iss = tcp_new_iss();
                    child_sk->snd_una = tsk->snd_una;
                    child_sk->rcv_nxt = tsk->rcv_nxt;
                    child_sk->snd_nxt = tsk->iss;
                    child_sk->rcv_wnd = tsk->rcv_wnd;

                    tcp_set_state(child_sk, TCP_SYN_RECV);
                    tcp_hash(child_sk);

                    tcp_send_control_packet(child_sk, TCP_SYN | TCP_ACK);

                    break;
                default:
					break;
            }
            break;
        case (TCP_SYN | TCP_ACK):
            if (tsk->state == TCP_SYN_SENT) 
                wake_up(tsk->wait_connect);
            break;
        case TCP_ACK:
            switch (tsk->state) 
			{
                case TCP_SYN_RECV:
                    tcp_sock_accept_enqueue(tsk);
                    wake_up(tsk->parent->wait_accept);
                    tcp_set_state(tsk, TCP_ESTABLISHED);
                    break;
                case TCP_ESTABLISHED:
                    if (tsk->nrstate == OPEN) 
                        wake_up(tsk->wait_send);
                    break;
                case TCP_FIN_WAIT_1:
                    tcp_set_state(tsk, TCP_FIN_WAIT_2);
                    break;
                case TCP_LAST_ACK:
                    tcp_set_state(tsk, TCP_CLOSED);
                    if (!tsk->parent) 
						tcp_bind_unhash(tsk);
                    tcp_unhash(tsk);
                    break;
                default:
                    break;
            }
            break;
        case (TCP_PSH | TCP_ACK):
            if (tsk->state == TCP_SYN_RECV) 
				tcp_set_state(tsk, TCP_ESTABLISHED);
            u32 seq_end = tsk->rcv_nxt;
            if (seq_end == cb->seq) 
			{
                while (ring_buffer_free(tsk->rcv_buf) < cb->pl_len) 
                {
                    wake_up(tsk->wait_read);
                    sleep_on(tsk->wait_recv);
                }
                write_ring_buffer(tsk->rcv_buf, cb->payload, cb->pl_len);
                tsk->rcv_wnd -= cb->pl_len;
                seq_end = cb->seq_end;
                struct ofo_buffer *entry, *q;
                list_for_each_entry_safe(entry, q, &tsk->rcv_ofo_buf, list) 
				{
                    if (seq_end < entry->seq) 
						break;
                    else 
					{
                        seq_end = entry->seq_end;
                        struct ring_buffer *rbuf = entry->tsk->rcv_buf;
                        int size = entry->pl_len;
                        while (ring_buffer_free(rbuf) < size) 
                        {
                            wake_up(tsk->wait_read);
                            sleep_on(tsk->wait_recv);
                        }
                        write_ring_buffer(entry->tsk->rcv_buf, entry->payload, entry->pl_len);
                        tsk->rcv_wnd -= entry->pl_len;
                        list_delete_entry(&entry->list);
                        free(entry->payload);
                        free(entry);
                    }
                }
                tsk->rcv_nxt = seq_end;
            } 
			else if (seq_end < cb->seq) 
                write_ofo_buffer(tsk, cb);
            else
            {
                tcp_send_control_packet(tsk, TCP_ACK);
                break;
            }
            if (tsk->wait_recv->sleep)
                wake_up(tsk->wait_recv);
            tcp_send_control_packet(tsk, TCP_ACK);
            if (tsk->wait_send->sleep && tsk->nrstate == OPEN) 
                wake_up(tsk->wait_send);
            break;
        case (TCP_ACK | TCP_FIN):
            if (tsk->state == TCP_FIN_WAIT_1) 
			{
                tcp_set_state(tsk, TCP_TIME_WAIT);
                tcp_send_control_packet(tsk, TCP_ACK);
                tcp_set_timewait_timer(tsk);
            } 
            break;
        case TCP_FIN:
            switch (tsk->state) 
			{
                case TCP_ESTABLISHED:
                    tcp_set_state(tsk, TCP_LAST_ACK);
                    tcp_send_control_packet(tsk, TCP_ACK | TCP_FIN);
                    tcp_set_timewait_timer(tsk);
                    break;
                case TCP_FIN_WAIT_2:
                    tcp_set_state(tsk, TCP_TIME_WAIT);
                    tcp_send_control_packet(tsk, TCP_ACK);
                    tcp_set_timewait_timer(tsk);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}
