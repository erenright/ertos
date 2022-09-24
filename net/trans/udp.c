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

/*
 * net/trans/udp.c
 *
 * An implemention of UDP as per RFC 768.
 */

#include "udp.h"
#include "ip.h"

#include <stdio.h>

// The UDP header
struct en_udp_hdr {
	struct {
		uint16_t	src;
		uint16_t	dst;
	} port;

	uint16_t	len;
	uint16_t	cksum;
} __attribute__((packed));

// The IP pseudo header
struct en_udp_pseudo_hdr {
	uint32_t	src;
	uint32_t	dst;
	uint8_t		zero;
	uint8_t		proto;
	uint16_t	udp_len;
} __attribute__((packed));

// It would be nice to be able to use pkt.h's ocksum16, but we have two
// buffers to check
static inline uint16_t udp_ocksum16(void *buf1, size_t len1, void *buf2, size_t len2)
{
	uint16_t *p = (uint16_t *)buf1;
	uint32_t sum = 0;

	while (len1 > 0) {
		sum += *p++;
		len1 -= 2;
	}

	p = (uint16_t *)buf2;
	while (len2 > 0) {
		sum += *p++;
		len2 -= 2;
	}

	sum = (sum & 0xffff) + (sum >> 16);

	return (uint16_t)sum;
}

/*---------------------------------------------------------------------
 * en_udp_input()
 *
 * 	This function:
 *
 * 	- handles UDP messages
 * 	- expects the packet to be an IPv4 packet
 * 	- expects the IP header to be present
 * 	- expects the packet to arrive in host byte order (IP only)
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
int en_udp_input(struct en_net_pkt *pkt)
{
	int rc = 0;
	struct en_ip_hdr *ip = (struct en_ip_hdr *)pkt->data;
	struct en_udp_hdr *udp = (struct en_udp_hdr *)(((void *)ip) + ip->ihl * 4);

#if 0
	struct en_udp_pseudo_hdr pseudo;
	uint16_t sum;

	// Only validate checksum if non-zero
	if (udp->cksum != 0) {
		// Generate pseudo header
		memset(&pseudo, 0, sizeof(pseudo));
		pseudo.src = ip->src;
		pseudo.dst = ip->dst;
		pseudo.proto = ip->proto;
		pseudo.udp_len = udp->len;

		sum = udp_ocksum16(&pseudo, sizeof(pseudo), udp, ip->len - (ip->ihl * 4));

		if (sum != udp->cksum) {
			printf("en_udp_input: dropped packet with bad checksum: is 0x%x s/b 0x%x\r\n",
				sum, udp->cksum);
		} else {
			printf("good\r\n");
		}
	}
#endif

	// Convert to host byte order
	udp->port.src = en_ntohs(udp->port.src);
	udp->port.dst = en_ntohs(udp->port.dst);
	udp->len = en_ntohs(udp->len);

	// Verify length
	if (udp->len != ip->len - (ip->ihl * 4)
		&& udp->len != pkt->len - (ip->ihl * 4)) {
		printf("en_udp_input: dropped packet with invalid length: %d\r\n", udp->len);
		rc = -1;
		goto out;
	}

	printf("Received UDP packet %d -> %d, len %d, cksum 0x%x\r\n",
		udp->port.src,
		udp->port.dst,
		udp->len,
		udp->cksum);

out:
	pkt_free(pkt);
	return rc;
}
