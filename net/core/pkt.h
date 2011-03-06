#ifndef _CORE_PKT_H
#define _CORE_PKT_H

#include <sys/list.h>
#include <types.h>

#define en_ntohs(x) ((uint16_t)( \
	  (((x) & 0xff00) >> 8) \
	| (((x) & 0x00ff) << 8) \
	))

#define en_ntohl(x) ((uint32_t)( \
	  (((x) & 0xff000000) >> 24) \
	| (((x) & 0x00ff0000) >> 8) \
	| (((x) & 0x0000ff00) << 8) \
	| (((x) & 0x000000ff) << 24) \
	))

#define en_htons(x) en_ntohs(x) 
#define en_htonl(x) en_ntohl(x) 

struct en_net_pkt {
	struct list list;

	void	*data;			/* Packet data			*/
	size_t	 len;			/* Length of packet data	*/
	size_t	 allocated;		/* Size of packet data buffer	*/

	struct en_eth_if *eth_if;	/* Interface that rx'd the pkt	*/

	// @@@ spinlock_t	lock;		/* Concurrency			*/
};

struct en_net_pkt * pkt_alloc(size_t);
void pkt_free(struct en_net_pkt *);
int pkt_add_head(struct en_net_pkt *, const void *, size_t);
int pkt_add_tail(struct en_net_pkt *pkt, const void *buf, size_t len);
int pkt_add(struct en_net_pkt *pkt, const void *hbuf, size_t hlen,
	const void *tbuf, size_t tlen);
int pkt_del_head(struct en_net_pkt *pkt, size_t len);

/*
 * Inlines
 */

/*---------------------------------------------------------------------
 * ocksum16()
 *
 * 	This function returns the 16-bit one's complement of the one's
 * 	complement for the buffer.  This function requires the supplied
 * 	buffer to be of a multiple of BUF_PADDING.
 *
 * Parameters:
 *
 * 	buf	- buffer to sum
 * 	len	- length of the buffer
 *
 * Returns:
 *
 * 	The checksum
 */
static inline uint16_t ocksum16(void *buf, size_t len)
{
	uint16_t *p = (uint16_t *)buf;
	uint32_t sum = 0;

	while (len > 0) {
		sum += *p++;
		len -= 2;
	}

	sum = (sum & 0xffff) + (sum >> 16);
	//printk("f1 %08x\n", sum);
	//sum = ~(sum + (sum >> 16)) & 0xffff;
	//printk("f2 %08x\n", sum);

	return (uint16_t)sum;
}

#endif /* ! _CORE_PKT_H */
