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

#ifndef _TRANS_IP_H
#define _TRANS_IP_H

#include <sys/list.h>
#include <types.h>

#include "../core/pkt.h"

struct ip_addr {
	uint32_t	addr;		/* IPv4 address		*/
	uint32_t	netmask;	/* Network mask		*/
	uint32_t	broadcast;	/* Broadcast address	*/
};

/*
 * Data types
 */

struct ip_desc {
	struct list list;

	struct ip_addr addr;		/* The IP address info	*/
};

/* The IPv4 header */
struct en_ip_hdr {
	/* Low bits are first */
	uint8_t		ihl:4;		/* Internet header length	*/
	uint8_t		version:4;	/* IP Version			*/

	uint8_t		tos;		/* Type of service		*/
	uint16_t	len;		/* Total length			*/
	uint16_t	id;		/* Identification		*/

	/* Low bits are first */
	uint16_t	frag_offset:13;	/* Fragmentation offset		*/
	uint16_t	flags:3;	/* Flags (see below)		*/

	uint8_t		ttl;		/* Time to live			*/
	uint8_t		proto;		/* Protocol			*/
	uint16_t	cksum;		/* Header checksum		*/

	uint32_t	src;		/* Source address		*/
	uint32_t	dst;		/* Destination address		*/
} __attribute__((packed));

#define IP_FLAG_DF	0x02		/* 0/1: Don't Frag/Frag		*/
#define IP_FLAG_MF	0x04		/* 0/1: More Frags/No Frags	*/

#define IP_PROTO_ICMP	0x01
#define IP_PROTO_UDP	0x11

/*
 * An IP route
 */
struct en_ip_route {
	struct list list;

	uint32_t	dst;		/* Destination host/network	*/
	uint32_t	netmask;	/* Network mask			*/
	uint32_t	gw;		/* Gateway address		*/

	uint8_t		flags;		/* Flags (see below)		*/
	uint8_t		metric;		/* Preference weight (less = better) */
	struct en_eth_if *eth_if;	/* Associated DLL interface	*/
};

#define IP_ROUTE_FLAG_UP	0x01	/* Route is up			*/
#define IP_ROUTE_FLAG_HOST	0x02	/* 0/1: route is host/network	*/
#define IP_ROUTE_FLAG_GW	0x04	/* Route is a gateway		*/

/*
 * Prototypes
 */
int en_ip_input(struct en_net_pkt *);
int en_ip_output(struct en_net_pkt *, uint32_t dst);
struct en_ip_route * en_ip_route_lookup(uint32_t dst);
int en_ip_route_add(struct en_ip_route *newrt);

#endif /* ! _TRANS_IP_H */
