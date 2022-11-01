/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Eric Enright
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/sched.h>

#include "eth.h"
#include "arp.h"

#include <sleep.h>
#include <stdio.h>

/*
 * Constants
 */

/*
 * Data types
 */

/*
 * Globals
 */

struct list eth_if_list = {
	.next = NULL,
	.prev = NULL
};

static struct completion rx_completion;

struct mac_addr mac_addr_bcast = {
	.addr = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};

// @@@ spinlock_t eth_if_list_lock = SPIN_LOCK_UNLOCKED;

/*---------------------------------------------------------------------
 * en_eth_tx_queue()
 *
 * 	This function runs over each Ethernet interface and outputs a frame.
 *
 * Parameters:
 *
 * 	arg	- unsused
 *
 * Returns:
 *
 * 	None
 */
static void en_eth_tx_task(void)
{
	struct list *p;
	struct en_eth_if *eth_if;
	struct en_net_pkt *pkt;

	while (1) {
		sleep(100);

		/* @@@ need list locking */

		/* Output one packet on each interface */
		for (p = eth_if_list.next; p != NULL; p = p->next) {
			eth_if = (struct en_eth_if *)p;

			if (!list_empty(&eth_if->tx_queue)) {
				pkt = (struct en_net_pkt *)eth_if->tx_queue.next;
				list_remove(pkt);

				eth_if->ops->xmit(pkt, eth_if);
			}
		}
	}
}

/*---------------------------------------------------------------------
 * en_eth_output()
 *
 * 	This function encapsulates and queues an Ethernet frame for output.
 *
 * Parameters:
 *
 * 	eth_if	- the interface to output on
 * 	pkt	- the packet (frame) to process
 * 	dst	- the target host
 * 	type	- the type of Ethernet frame to send
 *
 * Returns:
 *
 * 	None
 */
void en_eth_output(struct en_eth_if *eth_if, struct en_net_pkt *pkt,
			struct mac_addr *dst, int type)
{
	struct en_eth_mac_hdr hdr;

	memcpy(hdr.src, eth_if->addr.addr, sizeof(hdr.src));
	memcpy(hdr.dst, dst->addr, sizeof(hdr.dst));
	hdr.type = en_htons(type);

	pkt_add_head(pkt, &hdr, sizeof(hdr));

	// @@@ spin_lock_irq(&eth_if->tx_lock);
	list_add_after(&eth_if->tx_queue, pkt);
	// @@@ spin_unlock_irq(&eth_if->tx_lock);
}

/*---------------------------------------------------------------------
 * en_eth_input()
 *
 * 	This function processes a received Ethernet frame.
 *
 * Parameters:
 *
 * 	pkt	- the packet (frame) to process
 *
 * Returns:
 *
 * 	None
 */
static void en_eth_input(struct en_net_pkt *pkt)
{
	struct en_eth_mac_hdr *hdr = (struct en_eth_mac_hdr *)pkt->data;
	uint16_t type = en_ntohs(hdr->type);

	/* @@@ verify dst is us / promisc? */

	/* Strip the Ethernet header */
	pkt_del_head(pkt, sizeof(struct en_eth_mac_hdr));

	switch (type) {
	case ETH_TYPE_ARP:
		/* Pass to ARP module */
		en_arp_input(pkt);
		break;

	case ETH_TYPE_IPV4:
		/* Pass to IP module */
		en_ip_input(pkt);
		break;

	default:
		pkt_free(pkt);
		break;
	}
}

/*---------------------------------------------------------------------
 * en_eth_rx_drain()
 *
 * 	This function drains an interfaces rx queue.

 * Parameters:
 *
 * 	eth_if	- the interface to process
 *
 * Returns:
 *
 * 	None
 */
static inline void en_eth_rx_drain(struct en_eth_if *eth_if)
{
	struct en_net_pkt *pkt = NULL;

	do {
		pkt = NULL;

		/* Acquire a packet */
		// @@@ spin_lock_irq(&eth_if->rx_lock);
		if (!list_empty(&eth_if->rx_queue)) {
			pkt = (struct en_net_pkt *)eth_if->rx_queue.next;
			list_remove(&pkt->list);
		}
		// @@@ spin_unlock_irq(&eth_if->rx_lock);

		if (pkt != NULL) {
			/* Are we bound? */
			if (!eth_if->bound) {
				/* No, discard the packet */
				pkt_free(pkt);
				printf("%s: discarded frame on unbound interface\r\n", eth_if->name);
			} else {
				/* Yes, process it */
				en_eth_input(pkt);
			}
		}
	} while (pkt != NULL);
}

/*---------------------------------------------------------------------
 * en_rx_task()
 *
 * 	This function runs periodically and runs over the rx queue.
 *	Frames are dispatched or discarded as appropriate.
 *
 * Parameters:
 *
 * 	None
 *
 * Returns:
 *
 * 	None
 */
static void en_rx_task(void)
{
	struct list *p;
	struct en_eth_if *eth_if = NULL;

	while (1) {
		wait(&rx_completion);

		/* @@@ need list locking! */

		/* Process each interface */
		for (p = eth_if_list.next; p != NULL; p = p->next) {
			eth_if = (struct en_eth_if *)p;

			/* Process the queue */
			en_eth_rx_drain(eth_if);
		}
	}
}

/*---------------------------------------------------------------------
 * en_eth_register_if()
 *
 * 	This function registers a new Ethernet interface with the framework.
 *
 * Parameters:
 *
 * 	eth_if	- interface to register
 *
 * Returns:
 *
 * 	0 on success
 * 	-errno on error
 */
int en_eth_register_if(struct en_eth_if *eth_if)
{
	printf(__FILE__ ": registered %s\r\n", eth_if->name);

	/* Add this to our list of interfaces */
	list_add_after(&eth_if_list, &eth_if->list);

	/* Initialize the rx/tx queues */
	eth_if->rx_queue.next = NULL;
	eth_if->rx_queue.prev = NULL;
	// @@@ spin_lock_init(&eth_if->rx_lock);
	eth_if->tx_queue.next = NULL;
	eth_if->tx_queue.prev = NULL;
	// @@@ spin_lock_init(&eth_if->tx_lock);

	/* Initialize the IP address list */
	eth_if->ip_addrs.next = NULL;
	eth_if->ip_addrs.prev = NULL;
	// @@@ spin_lock_init(&eth_if->ip_lock);

	return 0;
}

/*---------------------------------------------------------------------
 * en_eth_rx()
 *
 * 	This function adds a new frame to the receive queue and
 *	arranges to have it processed.
 *
 * Parameters:
 *
 * 	eth_if	- interface to register
 *
 * Returns:
 *
 * 	0 on success
 * 	-errno on error
 */
// @@@
int sys_wake(uint32_t *args);
void en_eth_rx(struct en_eth_if *eth_if, struct en_net_pkt *pkt)
{
	pkt->eth_if = eth_if;

	/* @@@ lock? */
	// list_add_after(&pkt->list, &eth_if->rx_queue);
	list_add_after(&eth_if->rx_queue, &pkt->list);

	// @@@
	wake(&rx_completion);
}

/*---------------------------------------------------------------------
 * eth_init()
 *
 * 	This function initializes the Ethernet module.
 *
 * Parameters:
 *
 * 	None
 *
 * Returns:
 *
 * 	0 on success
 * 	-errno on error
 */
int eth_init(void)
{
	struct proc *p;

	rx_completion.wait = bfifo_alloc(1);
	if (rx_completion.wait == NULL) {
		printf("eth_init: bfifo_alloc failed\r\n");
		return -1;
	}

	// Start up tx and rx tasks
	p = spawn(en_eth_tx_task, "[eth_tx]", PROC_SYSTEM);
	if (p == NULL) {
		printf("eth_init: failed to spawn TX task\r\n");
		return -1;
	}

	p = spawn(en_rx_task, "[eth_rx]", PROC_SYSTEM);
	if (p == NULL) {
		printf("eth_init: failed to spawn RX task\r\n");
		return -1;
	}

	return 0;
}


