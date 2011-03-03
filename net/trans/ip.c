#include "ip.h"
#include "icmp.h"

#include <stdio.h>

/*
 * Globals
 */

static struct list en_ip_route_list = {
	.prev = NULL,
	.next = NULL
};

static int ip_id = 0;

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
	struct en_ip_route *rt;

	// First things firt, we need a route
	rt = en_ip_route_lookup(dst);
	if (rt == NULL) {
		printf("%x: no route to host\r\n", dst);
		return -1;
	}

	memset(&ip, 0, sizeof(ip));

	// Fill in header in network byte order
	ip.len = en_ntohs(pkt->len);
	ip.id = en_ntohs(ip_id);
	++ip_id;
	ip.frag_offset = 0;
	ip.cksum = en_ntohl(ip.cksum);
	ip.src = en_ntohl(ip.src);
	ip.dst = en_ntohl(ip.dst);

	//en_eth_output(rt->eth_if, pkt, @@@, ETH_TYPE_IPV4);

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
	if (ip->ihl < 5 || ip->len != pkt->len) {
		printf(__FILE__ ": dropped packet with invalid length\r\n");
		// @@@ rc = -EINVAL;
		return -1;
		goto out_free;
	}

	/* @@@ validate dst */

	switch (ip->proto) {
	case IP_PROTO_ICMP:
		en_icmp_input(pkt);
		break;

	default:
		printf(__FILE__ ": received unknown message: %x\r\n", ip->proto);
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
			return -1;
			goto out;
		}
	}

	/* The route does not exist; add it */
	list_add_after(&newrt->list, &en_ip_route_list);

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

