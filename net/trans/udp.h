/*
 * net/include/udp.h
 *
 * An implementation of UDP as per RFC 768.
 */

#ifndef _TRANS_UDP_H
#define _TRANS_UDP_H

#include "../core/pkt.h"

int en_udp_input(struct en_net_pkt *);

#endif // !_TRANS_UDP_H
