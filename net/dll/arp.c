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

#include <sys/kernel.h>
#include <stdio.h>

#include "eth.h"	/* @@@ en_ntohs */
#include "arp.h"

/*
 * ARP opcodes
 */
#define ARP_OP_REQUEST	1
#define ARP_OP_REPLY	2

/* Source hardware address offset into data */
#define ARP_SRC_HRD_OFFSET(arp) \
	(0)

/* Source protocol address offset into data */
#define ARP_SRC_PROTO_OFFSET(arp) \
	(arp->ar_hlen)

/* Destination hardware address offset into data */
#define ARP_DST_HRD_OFFSET(arp) \
	(arp->ar_hlen + arp->ar_plen)

/* Destination protocol address offset into data */
#define ARP_DST_PROTO_OFFSET(arp) \
	((arp->ar_hlen * 2) + arp->ar_plen)

struct list arp_cache_list = {
	.next = NULL,
	.prev = NULL
};

// @@@ static spinlock_t arp_cache_lock = SPIN_LOCK_UNLOCKED;

#if 0
static void dump_arp(struct en_arp_pkt *arp)
{
	int i;
	uint8_t *p = arp->data;

	/* Print MAC src/dst */
	i = ARP_SRC_HRD_OFFSET(arp);
	printf("src %x:%x:%x:%x:%x:%x ",
		p[i+0], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5]);
	i = ARP_DST_HRD_OFFSET(arp);
	printf("dst %x:%x:%x:%x:%x:%x\r\n",
		p[i+0], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5]);

	/* Print IP src/dst */
	i = ARP_SRC_PROTO_OFFSET(arp);
	printf("src %d.%d.%d.%d ",
		p[i+0], p[i+1], p[i+2], p[i+3]);
	i = ARP_DST_PROTO_OFFSET(arp);
	printf("dst %d.%d.%d.%d\r\n",
		p[i+0], p[i+1], p[i+2], p[i+3]);
}
#endif

/*---------------------------------------------------------------------
 * en_arp_cache_lookup()
 *
 * 	This function locates an ARP cache entry.
 *
 * Parameters:
 *
 * 	i_addr	- pointer to the IP address to lookup
 *
 * Returns:
 *
 *	A pointer to the cache entry, or NULL if not found.
 */
struct en_arp_entry * en_arp_cache_lookup(struct ip_addr *ip_addr)
{
	struct list *p;
	struct en_arp_entry *ep = NULL;

	/* Does this pair exist in the cache? */
	for (p = arp_cache_list.next; p != NULL; p = p->next) {
		ep = (struct en_arp_entry *)p;

		/* Hit? */
		if (ep->proto_addr.addr == ip_addr->addr) {
			/* Yes  */
			return ep;
		}
	}

	return NULL;
}

/*---------------------------------------------------------------------
 * en_arp_cache_add()
 *
 * 	This function adds a new entry to the ARP cache.
 *
 * Parameters:
 *
 * 	e	- pointer to the ARP entry
 *
 * Returns:
 *
 *	None
 */
static void inline en_arp_cache_add(struct en_arp_entry *e)
{
	e->created = clkticks;
	list_add_after(&arp_cache_list, e);
}

// Request a MAC address
int en_arp_request(struct en_eth_if *dev, uint32_t addr)
{
	struct en_net_pkt *pkt;
	struct en_arp_pkt *arp;
	struct en_arp_entry *entry;
	uint32_t ip;
	int arpsz = sizeof(struct en_arp_pkt) - 1
		+ (6 * 4)	// hlen
		+ (4 * 2);	// plen

	pkt = pkt_alloc(arpsz + sizeof(struct en_eth_mac_hdr));

	if (pkt == NULL) {
		printf("Unable to allocate ARP request\r\n");
		return -1;
	}

	arp = (struct en_arp_pkt *)pkt->data;
	entry = (struct en_arp_entry *)arp->data;

	// Construct ARP request
	arp->ar_hrd = en_htons(ARP_HRD_ETHERNET);
	arp->ar_proto = en_htons(ETH_TYPE_IPV4);
	arp->ar_hlen = 6;	// MAC byte size
	arp->ar_plen = 4;	// IPv4 byte size
	arp->ar_opcode = en_htons(ARP_OP_REQUEST);

	memset(arp->data + ARP_DST_HRD_OFFSET(arp), 0xFF, 6); // broadcast
	memcpy(arp->data + ARP_SRC_HRD_OFFSET(arp), dev->addr.addr, 6); //src
	ip = addr;
	ip = en_ntohl(ip);
	memcpy(arp->data + ARP_DST_PROTO_OFFSET(arp), &ip, 4); // required IP
	ip = ((struct ip_desc *)dev->ip_addrs.next)->addr.addr;
	ip = en_ntohl(ip);
	memcpy(arp->data + ARP_SRC_PROTO_OFFSET(arp),
		&ip,
		4); // first IP on this MAC

	// @@@ should use pkt_add_head
	pkt->len = arpsz;

	en_eth_output(dev, pkt, &mac_addr_bcast, ETH_TYPE_ARP);

	return 0;
}

/*---------------------------------------------------------------------
 * en_arp_input_request()
 *
 * 	This function:
 * 	- handles ARP REQUEST messages
 * 	- expects the packet header to have been converted to host byte order
 *
 * Parameters:
 *
 * 	pkt	- pointer to the ARP message
 *
 * Returns:
 *
 *	Zero on success
 * 	-errno on error
 */
static int en_arp_input_request(struct en_net_pkt *pkt)
{
	struct en_arp_pkt *arp = (struct en_arp_pkt *)pkt->data;
	struct ip_desc *ipd = NULL;
	struct list *p;
	struct en_eth_if *eth_if = NULL;
	uint8_t *ap = &arp->data[ARP_DST_PROTO_OFFSET(arp)];
	uint32_t addr = 0;
	struct mac_addr mac;

	/* Unpack the requested IP */
	addr = *ap++ << 24;
	addr |= *ap++ << 16;
	addr |= *ap++ << 8;
	addr |= *ap;

	/* Look up the IP address */
	/* @@@ should we lock this list here? we are in an interrupt
	 * so we should be safe from user context..? */
	for (p = eth_if_list.next; p != NULL; p = p->next) {
		eth_if = (struct en_eth_if *)p;

		ipd = en_eth_ip_lookup(eth_if, addr);
		if (ipd != NULL)
			break;
	}

	/* Did we find it? */
	if (ipd != NULL) {
		/* Yes, eth_if is the owning interface */
		//printf("%s: ARP request hit\r\n", eth_if->name);
		//dump_arp(arp);

		/* Refactor the request packet into a reply */
		arp->ar_opcode = ARP_OP_REPLY;
		memcpy(	arp->data + ARP_DST_HRD_OFFSET(arp),
			arp->data + ARP_SRC_HRD_OFFSET(arp),
			arp->ar_hlen);
		memcpy(	arp->data + ARP_DST_PROTO_OFFSET(arp),
			arp->data + ARP_SRC_PROTO_OFFSET(arp),
			arp->ar_plen);
		memcpy(	arp->data + ARP_SRC_HRD_OFFSET(arp),
			eth_if->addr.addr,
			arp->ar_hlen);
		addr = en_htonl(addr);
		memcpy(arp->data + ARP_SRC_PROTO_OFFSET(arp), &addr, sizeof(addr));

		//printf(__FILE__ ": reply with: \r\n");
		//dump_arp(arp);

		memcpy(mac.addr, arp->data + ARP_DST_HRD_OFFSET(arp), arp->ar_hlen);

		/* Convert endian */
		arp->ar_hrd = en_htons(arp->ar_hrd);
		arp->ar_proto = en_htons(arp->ar_proto);
		arp->ar_opcode = en_htons(arp->ar_opcode);

		/* Queue the packet for transmission */
		en_eth_output(pkt->eth_if, pkt, &mac, ETH_TYPE_ARP);
	} else {
		/* We do not have the IP */
		pkt_free(pkt);
	}

	return 0;
}

/*---------------------------------------------------------------------
 * en_arp_input_reply()
 *
 * 	This function:
 * 	- handles ARP REPLY messages
 * 	- expects the packet to have been converted to host byte order
 *	- adds the new entry to the ARP cache if it does not already exist
 *
 * Parameters:
 *
 * 	req	- pointer to the ARP message
 *
 * Returns:
 *
 *	Zero on success
 * 	-errno on error
 *		-ENOMEM	memory for the ARP cache could not be allocated
 */
static int en_arp_input_reply(struct en_net_pkt *pkt)
{
	struct en_arp_pkt *arp = (struct en_arp_pkt *)pkt->data;
	struct en_arp_entry *ep = NULL;
	struct en_arp_entry e;
	int rc = 0;

	memset(&e, 0, sizeof(e));

	memcpy(e.hrd_addr.addr, &arp->data[ARP_SRC_HRD_OFFSET(arp)], 6);
	memcpy(&e.proto_addr.addr, &arp->data[ARP_SRC_PROTO_OFFSET(arp)], 4);
	e.proto_addr.addr = en_ntohl(e.proto_addr.addr);

	//printf(__FILE__ ": received ARP reply:\n");
	//dump_arp(arp);

	// @@@ spin_lock_irq(&arp_cache_lock);

	/* Does this pair exist in the cache? */
	ep = en_arp_cache_lookup(&e.proto_addr);
	if (ep == NULL) {
		/* No, add it */
		ep = malloc(sizeof(struct en_arp_entry));
		if (ep == NULL) {
			// @@@ rc = -ENOMEM;
			rc = -1;
		} else {
			memcpy(ep, &e, sizeof(e));
			en_arp_cache_add(ep);
		}
	}

	// @@@ spin_unlock_irq(&arp_cache_lock);

	pkt_free(pkt);

	return rc;
}

/*---------------------------------------------------------------------
 * en_arp_input()
 *
 * 	This function:
 *
 * 	- handles ARP messages
 * 	- expects the packet to arrive in network byte order
 * 	- frees the packet
 *
 * Parameters:
 *
 * 	pkt	- pointer to the Ethernet frame
 *
 * Returns:
 *
 *	Zero on success
 * 	-errno on error:
 * 		-EINVAL	- the packet is malformed
 */
int en_arp_input(struct en_net_pkt *pkt)
{
	int rc = 0;
	struct en_arp_pkt *arp = (struct en_arp_pkt *)pkt->data;

	/* Convert packet to host byte order */
	arp->ar_hrd = en_ntohs(arp->ar_hrd);
	arp->ar_proto = en_ntohs(arp->ar_proto);
	arp->ar_opcode = en_ntohs(arp->ar_opcode);

	/* Validate length fields */
	if (sizeof(struct en_arp_pkt) - 1
		+ (arp->ar_hlen * 2) + (arp->ar_plen * 2)
		> pkt->len) {

		/* Avoid buffer overrun */
		printf("en_arp_input: invalid length: %d\r\n", pkt->len);
		// @@@ rc = -EINVAL;
		rc = -1;
		goto out;
	}

	/* We only support Ethernet */
	if (arp->ar_hrd != ARP_HRD_ETHERNET) {
		printf(__FILE__ ": discarding unknown ARP hrd: %x\r\n", arp->ar_hrd);
		// @@@ rc = -EPROTONOSUPPORT;
		rc = -2;
		goto out;
	}

	/* We only support IP */
	if (arp->ar_proto != ETH_TYPE_IPV4) {
		printf(__FILE__ ": discarding unknown ARP proto: %x\r\n", arp->ar_proto);
		// @@@ rc = -EPROTONOSUPPORT;
		rc = -2;
		goto out;
	}

	if (arp->ar_hlen != 6 || arp->ar_plen != 4) {
		printf(__FILE__ ": discard ARP packet, invalid hlen or plen (%d, %d)\r\n",
		arp->ar_hlen, arp->ar_plen);
		// @@@ rc = -EINVAL;
		rc = -1;
		goto out;
	}

	switch (arp->ar_opcode) {
	case ARP_OP_REQUEST:
		en_arp_input_request(pkt);
		break;

	case ARP_OP_REPLY:
		en_arp_input_reply(pkt);
		break;

	default:
		printf(__FILE__ ": received unknown message: %x\n", arp->ar_opcode);
		pkt_free(pkt);
		break;
	}

out:

	return rc;
}
