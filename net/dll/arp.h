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
