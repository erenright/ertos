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

#ifndef _DLL_ARP_H
#define _DLL_ARP_H

#include <sys/list.h>
#include <types.h>

#include "eth.h"
#include "../trans/ip.h"

/*
 * Data types
 */
struct en_arp_pkt {
	uint16_t	ar_hrd;		/* Hardware address space	*/
	uint16_t	ar_proto;	/* Protocol address space	*/
	uint8_t		ar_hlen;	/* Length of hardware addresses */
	uint8_t		ar_plen;	/* Length of protocol addresses */
	uint16_t	ar_opcode;	/* Opcode (REQUEST | REPLY)	*/
	uint8_t		data[1];	/* Data, variable length	*/
} __attribute__((packed));

struct en_arp_entry {
	struct list list;

	struct mac_addr	hrd_addr;	/* MAC address			*/
	struct ip_addr	proto_addr;	/* IP address			*/

	unsigned long	created;	/* Time created (jiffies)	*/
};

struct list arp_cache_list;

int en_arp_input(struct en_net_pkt *pkt);
struct en_arp_entry * en_arp_cache_lookup(struct ip_addr *ip_addr);
int en_arp_request(struct en_eth_if *dev, uint32_t addr);

#define ARP_HRD_ETHERNET	1

#endif /* ! _DLL_ARP_H */
