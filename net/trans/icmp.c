#include "../core/pkt.h"
#include "ip.h"

#include <stdio.h>

/* The ICMP header */
struct en_icmp_hdr {
	uint8_t		type;		/* Type				*/
	uint8_t		code;		/* Code				*/
	uint16_t	cksum;		/* Checksum			*/
	uint16_t	id;		/* Identifier			*/
	uint16_t	seqnum;		/* Sequence number		*/
} __attribute__((packed));

#define ICMP_TYPE_ECHO		8
#define ICMP_TYPE_ECHO_REPLY	0

/*---------------------------------------------------------------------
 * en_icmp_input()
 *
 * 	This function:
 *
 * 	- handles ICMP messages
 * 	- expects the packet to be an IPv4 packet
 * 	- expects the packet to arrive in host byte order (IP only)
 * 	- frees the packet if it does not result in a response
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
int en_icmp_input(struct en_net_pkt *pkt)
{
	int rc = 0;
	struct en_ip_hdr *ip = (struct en_ip_hdr *)pkt->data;
	struct en_icmp_hdr *icmp = (struct en_icmp_hdr *)(((void *)ip) + ip->ihl * 4);
	uint16_t sum;
	uint32_t dst;
	int mustfree = 1;

	/* Validate checksum */
	sum = ocksum16(icmp, pkt->len - (ip->ihl * 4));

	if (sum != 0xffff) {
		printf(__FILE__ ": dropped packet with bad cksum (%x sb %x)\r\n", icmp->cksum, sum);
		// @@@ rc = -EINVAL;
		rc = -1;
		goto out;
	}

	/* Convert to host byte order */
	icmp->cksum = en_ntohs(icmp->cksum);
	icmp->id = en_ntohs(icmp->id);
	icmp->seqnum = en_ntohs(icmp->seqnum);

	switch (icmp->type) {
	case ICMP_TYPE_ECHO:
		printf(__FILE__ ": received echo %x %x\r\n", icmp->id, icmp->seqnum);
		dst = ip->src;
		icmp->type = ICMP_TYPE_ECHO_REPLY;
		icmp->id = en_htons(icmp->id);
		icmp->seqnum = en_htons(icmp->seqnum);
		sum = ocksum16(icmp, pkt->len - (ip->ihl * 4));
		icmp->cksum = en_htons(sum);

		// ip and icmp data pointers become invalid after this
		pkt_del_head(pkt, sizeof(struct en_icmp_hdr));

		en_ip_output(pkt, dst);

		// Do not free this packet as it has moved on to output
		mustfree = 0;
		break;

	case ICMP_TYPE_ECHO_REPLY:
		printf(__FILE__ ": received echo reply %x %x\r\n", icmp->id, icmp->seqnum);
		break;

	default:
		printf(__FILE__ ": received unknown message: %x\r\n", icmp->type);
		break;
	}

out:
	if (mustfree)
		pkt_free(pkt);
	return rc;
}
