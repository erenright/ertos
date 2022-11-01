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

#include "ip.h"
#include "icmp.h"
#include "udp.h"
#include "../dll/arp.h"

#include <stdio.h>

#define IP_TTL	64

/*
 * Globals
 */

static struct list en_ip_route_list = {
	.prev = NULL,
	.next = NULL
};

/*static struct list en_ip_pkt_arp_queue = {
	.prev = NULL,
	.next = NULL
};

static struct list en_ip_pkt_output_queue = {
	.prev = NULL,
	.next = NULL
};*/

// @@@ static spinlock_t en_ip_route_lock = SPIN_LOCK_UNLOCKED;

/*---------------------------------------------------------------------
 * en_ip_input()
 *
 * 	This function:
 *
 * 	- outputs IP messages
 * 	- expects the IP portion of the packet to be in host byte order
 *
 * Parameters:
 *
 * 	pkt	- pointer to the IP packet
 *	dst	- destination address
 *
 * Returns:
 *
 *	Zero on success
 * 	-errno on error:
 * 		-EINVAL	- the packet is malformed
 *		-???    - no route
 */
int en_ip_output(struct en_net_pkt *pkt, uint32_t dst)
{
	struct en_ip_hdr ip;
	struct en_ip_route *rt = NULL;
	struct en_arp_entry *arp;
	uint16_t sum;

	// First things firt, we need a route
	rt = en_ip_route_lookup(dst);
	if (rt == NULL) {
		printf("%x: no route to host\r\n", dst);
		return -1;
	}

	memset(&ip, 0, sizeof(ip));

	// Fill in header in network byte order
	ip.ihl = 5;	// 20 bytes
	ip.version = 4;	// IPv4
	ip.len = en_ntohs(pkt->len + sizeof(struct en_ip_hdr)); // @@@ use pkt_add to add the header to the packet immediately, and then initialize pointers for this work
	ip.ttl = IP_TTL;
	ip.proto = IP_PROTO_ICMP;
	memcpy(&ip.src,&((struct ip_desc *)rt->eth_if->ip_addrs.next)->addr, 4);
	ip.src = en_htonl(ip.src);
	ip.dst = en_htonl(dst);
	sum = ocksum16(&ip, ip.ihl * 4);
	ip.cksum = ~sum;	// invert

	arp = en_arp_cache_lookup((struct ip_addr *)&dst);
	if (arp == NULL) {
		// Cache miss, request ARP lookup
		en_arp_request(rt->eth_if, dst);

		// @@@ FIXME queue packet
	} else {
		// Good to go, output packet
		pkt_add_head(pkt, &ip, sizeof(ip));
		en_eth_output(rt->eth_if, pkt, &arp->hrd_addr, ETH_TYPE_IPV4);
	}

	return 0;
}

/*---------------------------------------------------------------------
 * en_ip_input()
 *
 * 	This function:
 *
 * 	- handles IP messages
 * 	- expects the packet to arrive in network byte order
 * 	- frees the packet
 *
 * Parameters:
 *
 * 	pkt	- pointer to the IP packet
 *
 * Returns:
 *
 *	Zero on success
 * 	-errno on error:
 * 		-EINVAL	- the packet is malformed
 */
int en_ip_input(struct en_net_pkt *pkt)
{
	int rc = 0;
	struct en_ip_hdr *ip = (struct en_ip_hdr *)pkt->data;
	uint16_t sum;

	/* Avoid overrun on checksum */
	if (ip->ihl * 4 > pkt->len) {
		printf(__FILE__ ": dropped packet with invalid length: %d\r\n", pkt->len);
		// @@@ rc = -EINVAL;
		return -1;
		goto out_free;
	}

	/* Validate checksum */
	sum = ocksum16(ip, ip->ihl * 4);

	if (sum != 0xffff) {
		printf(__FILE__ ": dropped packet with bad cksum (%x sb %x)\r\n", ip->cksum, sum);
		// @@@ rc = -EINVAL;
		return -1;
		goto out_free;
	}

	/* Convert packet to host byte order */
	ip->len = en_ntohs(ip->len);
	ip->id = en_ntohs(ip->id);
	/* @@@ ip->frag_offset */
	ip->cksum = en_ntohl(ip->cksum);
	ip->src = en_ntohl(ip->src);
	ip->dst = en_ntohl(ip->dst);

	/* Validate length fields */

	// If the pkt length is 46 (60 byte Ethernet minimum minus Ethernet
	// header), then force the pkt len to be the IP len.  This is
	// necessary because the Ethernet layer does not strip out padding.
	if (pkt->len == 46 && ip->len < 46)
		pkt->len = ip->len;

	if (ip->ihl < 5 || ip->len != pkt->len) {
		printf(__FILE__ ": dropped packet with invalid length\r\n");
		printf("ip->ihl: %d\r\n", ip->ihl);
		printf("ip->len: %d\r\n", ip->len);
		printf("pkt->len: %d\r\n", pkt->len);
		// @@@ rc = -EINVAL;
		return -1;
		goto out_free;
	}

	/* @@@ validate dst */

	switch (ip->proto) {
	case IP_PROTO_ICMP:
		en_icmp_input(pkt);
		break;

	case IP_PROTO_UDP:
		en_udp_input(pkt);
		break;

	default:
		break;
	}

	return rc;

out_free:
	pkt_free(pkt);
	return rc;
}

/*---------------------------------------------------------------------
 * en_ip_route_add()
 *
 * 	This function:
 *
 * 	- adds the specified route to the routing table
 *
 * Parameters:
 *
 * 	rt	- pointer to the route
 *
 * Returns:
 *
 *	Zero on success
 * 	-errno on error:
 * 		-EEXIST	- the route (dst/netmask/if pair) already exists
 */
int en_ip_route_add(struct en_ip_route *newrt)
{
	int rc = 0;
	struct list *p;
	struct en_ip_route *rt;

	// @@@ spin_lock_irq(&en_ip_route_lock);

	/* Does the route already exist? */
	for (p = en_ip_route_list.next; p != NULL; p = p->next) {
		rt = (struct en_ip_route *)p;

		if (rt->dst == newrt->dst
			&& rt->netmask == newrt->netmask
			&& rt->eth_if == newrt->eth_if) {

			/* Yes, forbid addition */
			// @@@ rc = -EEXIST;
			rc = -1;
			goto out;
		}
	}

	/* The route does not exist; add it */
	list_add_after(&en_ip_route_list, newrt);

out:
	// @@@ spin_unlock_irq(&en_ip_route_lock);

	return rc;
}

/*---------------------------------------------------------------------
 * en_ip_route_del()
 *
 * 	This function:
 *
 * 	- removes the specified route to the routing table
 *
 * Parameters:
 *
 * 	rt	- pointer to the route
 *
 * Returns:
 *
 * 	None
 */
void en_ip_route_del(struct en_ip_route *rt)
{
	// @@@ spin_lock_irq(&en_ip_route_lock);
	list_remove(&rt->list);
	// @@@ spin_unlock_irq(&en_ip_route_lock);
}

/*---------------------------------------------------------------------
 * en_ip_route_lookup()
 *
 * 	This function:
 *
 * 	- looks up a route for the specified IP address
 *
 * Parameters:
 *
 * 	dst	- the destination address
 *
 * Returns:
 *
 * 	A pointer to the route, or NULL if none found.
 */
struct en_ip_route * en_ip_route_lookup(uint32_t dst)
{
	struct list *p;
	struct en_ip_route *rt;
	struct en_ip_route *bestrt = NULL;

	// @@@ spin_lock_irq(&en_ip_route_lock);

	/* Look for a route */
	for (p = en_ip_route_list.next; p != NULL; p = p->next) {
		rt = (struct en_ip_route *)p;

		/* Does the destination match this route? */
		if ((dst & rt->netmask) == rt->dst) {
			/* Yes.  Have we found a route already? */
			if (bestrt != NULL) {
				/* Yes, is this one better? */
				if (rt->metric < bestrt->metric) {
					/* Yes, use the new one */
					bestrt = rt;
				}
			} else {
				/* No, record this one */
				bestrt = rt;
			}
		}
	}

	// @@@ spin_unlock_irq(&en_ip_route_lock);

	/* @@@ what will happen if someone deletes this after we return ?! */
	/* (badness, thats what) */

	return bestrt;
}

static void ip_tx_task(void)
{
	while (1) {
		// Wait until we have a job to perform

		// Run the arp queue first since it may result in an addition
		// to the output queue
	
		// Run the output queue
	}
}

int en_ip_init(void)
{
	struct proc *p;

	// Start up the IP tx task
	p = spawn(ip_tx_task, "[ip_tx]", PROC_SYSTEM);
	if (p == NULL) {
		printf("Failed to spawn ip_tx task\r\n");
		return -1;
	}

	return 0;
}
