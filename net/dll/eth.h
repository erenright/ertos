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
 * this list of conditions and the following disclaimer.
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

#ifndef _DLL_ETH_H
#define _DLL_ETH_H

#include <sys/list.h>
#include <types.h>

#include "../core/pkt.h"
#include "../trans/ip.h"

/*
 * Constants
 */

#define EN_ETH_ADDRSIZE	(8 * 6)		/* 48-bit MAC */
#define EN_ETH_IF_NAMESIZE	8

#define EN_ETH_LINK_AUTO	0
#define EN_ETH_LINK_10FDX	1
#define EN_ETH_LINK_10HDX	2
#define EN_ETH_LINK_100FDX	3
#define EN_ETH_LINK_100HDX	3

#define SIOCDEVPRIVATE		0	// @@@

#define EN_ETH_SET_MAC		(SIOCDEVPRIVATE + 0x1)
#define EN_ETH_GET_MAC		(SIOCDEVPRIVATE + 0x2)
#define EN_ETH_SET_IP		(SIOCDEVPRIVATE + 0x3)
#define EN_ETH_GET_IP		(SIOCDEVPRIVATE + 0x4)
#define EN_ETH_REMOVE_IP	(SIOCDEVPRIVATE + 0x5)
#define EN_ETH_GET_LINK_STATUS	(SIOCDEVPRIVATE + 0x6)
#define EN_ETH_SET_LINK_SPEED	(SIOCDEVPRIVATE + 0x7)
#define EN_ETH_GET_LINK_SPEED	(SIOCDEVPRIVATE + 0x8)
#define EN_ETH_GET_STATS	(SIOCDEVPRIVATE + 0x9)
#define EN_ETH_BIND		(SIOCDEVPRIVATE + 0xA)
#define EN_ETH_RELEASE		(SIOCDEVPRIVATE + 0xB)

/*
 * Data types
 */

/* Generic type for a MAC address */
struct mac_addr {
	uint8_t addr[EN_ETH_ADDRSIZE];
};

/* Ethernet interface statistics */
struct en_eth_stats {
	unsigned int rx_bytes;		/* Bytes received */
	unsigned int tx_bytes;		/* Bytes transmitted */
	unsigned int rx_frames;		/* Frames received */
	unsigned int tx_frames;		/* Frames transmitted */

	unsigned int runts;		/* Runts received */
	unsigned int oversized;		/* Oversized frames received */
	unsigned int fcs_errors;	/* Number of FCS failures */
};

/* Ethernet device operations */
struct en_eth_if;
struct en_eth_ops {
	int (*xmit)(struct en_net_pkt *, struct en_eth_if *);	/* Transmit frame */
	int (*open)(struct en_eth_if *);	/* Open the device */
	int (*release)(struct en_eth_if *);	/* Close/release the device */
};

/* Ethernet interface data type */
struct en_eth_if {
	/*
	 * Kernel interfaces
	 */
	struct list list;

	int dev_major;			/* /dev major number		*/
	int dev_minor;			/* /dev minor number		*/
	struct en_eth_ops *ops;		/* Ethernet operations		*/

	/* @@@ */
	unsigned long base_addr;
	unsigned int irq;

	/*
 	 * Interface properties
 	 */
	char name[EN_ETH_IF_NAMESIZE];	/* Interface name @@@		*/
	struct mac_addr addr;		/* MAC address			*/
	unsigned short mtu;		/* Maximum transmission unit	*/
	struct en_eth_stats stats;	/* Transmission statistics	*/

	void *priv;			/* Hardware driver private	*/

	char bound;			/* Whether or not the device is bound */
	void *owner;			/* Who bound the device, fd or NET */

	struct list      rx_queue;	/* Receive queue		*/
	// @@@ spinlock_t       rx_lock;	/* Receive queue lock		*/
	struct list      tx_queue;	/* Transmit queue		*/
	// @@@ spinlock_t       tx_lock;	/* Transmit queue lock		*/

	struct list      ip_addrs;	/* Assigned IP Addresses	*/
	// @@@ spinlock_t       ip_lock;	/* IP Address lock		*/
};

/* The MAC header */
struct en_eth_mac_hdr {
	uint8_t		dst[6];
	uint8_t		src[6];
	uint16_t	type;
} __attribute__((packed));

/* en_eth_if.bound - What, if anything, has the interface bound */
#define EN_ETH_UNBOUND		0x00
#define EN_ETH_BOUND_NET	0x01
#define EN_ETH_BOUND_FD		0x02	/* en_eth_if.owner = &filp */

/* Ethernet frame types */
#define ETH_TYPE_IPV4		0x0800	/* IPv4 */
#define ETH_TYPE_ARP		0x0806	/* ARP */

/*
 * Prototypes
 */

int en_eth_register_if(struct en_eth_if *);
void en_eth_rx(struct en_eth_if *, struct en_net_pkt *);
void en_eth_output(struct en_eth_if *eth_if, struct en_net_pkt *pkt,
			struct mac_addr *dst, int type);

/*
 * Globals
 */
extern struct list eth_if_list;
// @@@ extern spinlock_t eth_if_list_lock;
struct mac_addr mac_addr_bcast;

/*
 * Inlines
 */

static inline struct ip_desc * en_eth_ip_lookup(struct en_eth_if *eth_if,
	uint32_t addr)
{
	struct list *p;
	struct ip_desc *ipd = NULL;

	/* Does address exist? */
	for (p = eth_if->ip_addrs.next; p != NULL; p = p->next) {
		ipd = (struct ip_desc *)p;

		/* Hit? */
		if (addr == ipd->addr.addr) {
			/* Yes  */
			return ipd;
		}
	}

	return NULL;
}

#endif /* ! _DLL_ETH_H */
