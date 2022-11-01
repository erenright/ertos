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

/*
 * arch/eth.c
 *
 * Device driver for EP9301 Ethernet Controller
 */

#include "regs.h"

#include <types.h>
#include <sys/mii.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/irq.h>

#include <sleep.h>
#include <stdio.h>

// @@@
#include "../net/dll/eth.h"

#define MIICmd_Write	(1<<14)
#define MIICmd_Read	(1<<15)

// MIISts bits
#define MIISts_Busy	(1)

#define MDCDIV		7		// MDC divisor

#define PHYAD		(1<<5)

// Receive Descriptors
struct rx_desc {
	uint32_t	RxBufAdr;

	uint32_t	BufferLength	: 16;
	uint32_t	BufferIndex	: 15;
	uint32_t	NotSOF		: 1;
};

// Receive Status
struct rx_sts {
	uint32_t	Status		: 31;
	uint32_t	RFP_Hi		: 1;

	uint32_t	FrameLength	: 16;
	uint32_t	BufferIndex	: 15;
	uint32_t	RFP_Lo		: 1;
};

// Transmit Descriptors
struct tx_desc {
	uint32_t	TxBufAdr;

	uint32_t	BufferLength	: 12;
	uint32_t	BufferCmd	: 4;
	uint32_t	BufferIndex	: 15;
	uint32_t	EOF		: 1;
};

// Transmit Status
struct tx_sts {
	uint32_t	BufferIndex	: 15;
	uint32_t	FrameStatus	: 15;
	uint32_t	TxFP		: 1;
	uint32_t	TxWE		: 1;
};

#define RX_BUF_SIZE	1520	// 1518 (ethernet frame) + 2 (for alignment)
#define RX_DESC_NUM	64	// number of descriptors
#define RX_STS_NUM	RX_DESC_NUM

// match tx buffer sizes for no reason other than consistency
#define TX_BUF_SIZE	RX_BUF_SIZE
#define TX_DESC_NUM	RX_DESC_NUM
#define TX_STS_NUM	TX_DESC_NUM

struct rx_desc rx_desc[RX_DESC_NUM];
struct rx_sts  rx_sts[RX_STS_NUM];
char *rx_buf = NULL;

struct tx_desc tx_desc[TX_DESC_NUM];
struct tx_sts  tx_sts[TX_STS_NUM];
char *tx_buf = NULL;

static struct en_eth_if eth_if;

struct proc *eth_task = NULL;

static void eth_isr(void);

// Write data to an MII register
static inline void mii_write(uint16_t _reg, uint16_t data)
{
	uint32_t reg = _reg & (MIICmd_REGAD | MIICmd_PHYAD);
	reg |= MIICmd_Write | PHYAD;

	outl(MIIData, data);
	outl(MIICmd, reg);
	while (inl(MIISts) & MIISts_Busy);
}

// Read data from an MII register
static inline uint16_t mii_read(uint16_t _reg)
{
	uint16_t val;
	uint32_t reg = _reg & (MIICmd_REGAD | MIICmd_PHYAD);
	reg |= MIICmd_Read | PHYAD;

	outl(MIICmd, reg);
	while (inl(MIISts) & MIISts_Busy);
	val = inl(MIIData);

	return val;
}

static int eth_reset(struct en_eth_if *dev)
{
	uint32_t self;

	// Step 0.1 - Reset MAC
	outl(SelfCtl, (inl(SelfCtl) | SelfCtl_RESET));
	while (inl(SelfCtl) & SelfCtl_RESET);

	// Step 0.2 - Reset PHY
	mii_write(MII_CONTROL, MII_CONTROL_RESET);
	while (mii_read(MII_CONTROL) & MII_CONTROL_RESET);

	printf("MAC+PHY reset, AutoNG enabled\r\n");

	// Step 1 - Determine available PHY
	// Always "1" on TS-7250

	// Step 2 - Enable AutoNG
	self = (MDCDIV << SelfCtl_MDCDIV_SHIFT);	// 32 divisor
	outl(SelfCtl, self);

	mii_write(MII_AUTONG_ADV, 0x01E1);// @@@ set 100M/10M Full/Half ability?

	mii_write(MII_CONTROL, MII_CONTROL_AUTONGEN | MII_CONTROL_RESTART_AUTONG);

	// Wait for AutoNG to complete
	printf("Waiting for Ethernet link...\r\n");
	while (!(mii_read(MII_STATUS) & MII_STATUS_AUTONG)) {
		sleep(250);
	}

	self |= SelfCtl_PSPRS;	// Enable preamble suppress
	outl(SelfCtl, self);

	// Step 3 - Set RXDQBAdd and RXDCurAdd to point to the start of the
	// rx desc queue
	outl(RXDQBAdd, rx_desc);
	outl(RXDCurAdd, rx_desc);

	// Step 4 - Set RXDQBLen to the length of the rx desc queue
	outw(RXDQBLen, RX_DESC_NUM * sizeof(struct rx_desc));

	// Step 5 - Set RXStsQBAdd and RXStsQCurAdd to point to the start of
	// the receive status queue
	outl(RXStsQBAdd, rx_sts);
	outl(RXStsQCurAdd, rx_sts);

	// Step 6 - Set RXStsQBLen to the length of the status queue
	outw(RXStsQBLen, RX_STS_NUM * sizeof(struct rx_sts));

	// Step 7 - Set BMCtl.RxEn which clears the RXDEnq/RXStsEnq registers
	// and initializes internal pointers to the queue.
	outl(BMCtl, (inl(BMCtl) | BMCtl_RxEn));

	// Step 8 - Set TXDQBAdd and TXDQCurAdd to point to the start of the
	// transmit descriptor queue
	outl(TXDQBAdd, tx_desc);
	outl(TXDQCurAdd, tx_desc);

	// Step 9 - Set TXDQBLen to the length of the transmit descriptor queue
	outl(TXDQBLen, TX_DESC_NUM * sizeof(struct tx_desc));

	// Step 10 - Set TXStsQBAdd and TXStsQCurAdd to point to the start of
	// the transmit status queue
	outl(TXStsQBAdd, tx_sts);
	outl(TXStsQCurAdd, tx_sts);

	// Step 11 - Set TXStsQBLen to the length of the status queue
	outw(TXStsQBLen, TX_STS_NUM * sizeof(struct tx_sts));

	// Step 12 - Set BMCtl.TxEn which clears the TXDEnq and initializes
	// internal pointers to the queues.
	outl(BMCtl, (inl(BMCtl) | BMCtl_TxEn));

	// Step 13 - Set required interrupt mask and global interrupt mask
	// Interrupt on RX buffer operations, and register IRQ handler
	if (register_irq_handler(dev->irq, eth_isr, 0))
		printf("Failed to register MAC IRQ handler\r\n");
	// @@@ outl(IntEn, (IntEn_REOFIE | IntEn_REOBIE | IntEn_RHDRIE));
	//outl(IntEn, (IntEn_REOFIE | IntEn_REOBIE | IntEn_RHDRIE |IntEn_PHYSIE));
	//inl(IntStsC);			// Clear pending interrupts
	//outl(GIntMsk, GIntMsk_INT);	// Enable interrupts

	// Step 14 - Wait for BMSts.RxAct to be set, and then enqueue the
	// receive descriptors and status.
	while (!(inl(BMSts) & BMSts_RxAct));
	outl(RXDEnq, RX_DESC_NUM);
	outl(RXStsEnq, RX_STS_NUM);

	// Step 15 - Set the required values for Individial Addresses and
	// Hash Table
	// @@@ already initialized by RedBoot (?)

	// Step 16 - Set the required options in RXCtl and TXCtl, enabling
	// SRxON and STxON
	// Accept broadcast and direct frames
	outl(RXCtl, (RXCtl_SRxON | RXCtl_BA | RXCtl_IA0 | RXCtl_RCRCA));
	//outl(RXCtl, (RXCtl_SRxON | RXCtl_BA | RXCtl_IA0 | RXCtl_PA | RXCtl_RCRCA));
	outl(TXCtl, TXCtl_STxON);

	outl(IntEn, (IntEn_REOFIE | IntEn_REOBIE | IntEn_RHDRIE |IntEn_PHYSIE));
	inl(IntStsC);			// Clear pending interrupts
	outl(GIntMsk, GIntMsk_INT);	// Enable interrupts
	enable_irq(dev->irq);

	// Step 17 - Set any required options in the PHY, and activate

    return 0;
}

static int ep9301_xmit(struct en_net_pkt *pkt, struct en_eth_if *dev)
{
	static struct tx_desc *next = tx_desc;

	if (pkt->len > 1518) {
		printf("ep9301_xmit: attempted to xmit oversized frame\r\n");
		return -1;
	}

	next->EOF = 1;
	next->BufferLength = pkt->len;
	memcpy((char *)next->TxBufAdr, pkt->data, pkt->len);

	dev->stats.tx_frames++;
	dev->stats.tx_bytes += pkt->len;

	pkt_free(pkt);

	// Handle wraparound
	if (++next >= &tx_desc[TX_DESC_NUM])
		next = tx_desc;

	outl(TXDEnq, 1);

	return 0;
}

static void process_tx_queue(struct en_eth_if *dev)
{
	static struct tx_sts *last = &tx_sts[TX_STS_NUM];
	struct tx_sts *cur = (struct tx_sts *)inl(TXStsQCurAdd);
	int q = 0;
	
	do {
		++q;

		if (!last->TxWE)
			printf("frame xmit failed: %x\r\n", last->FrameStatus);

		// Handle wraparound
		if (++last > &tx_sts[TX_STS_NUM - 1])
			last = tx_sts;
	} while (last != cur && q < TX_STS_NUM);
}

static void process_rx_queue(struct en_eth_if *dev)
{
	static struct rx_sts *last = rx_sts;
	struct rx_sts *cur;
	int q = 0;

	struct en_net_pkt *pkt;

	cur = (struct rx_sts *)inl(RXStsQCurAdd);

	do {
		++q;

		dev->stats.rx_frames++;
		dev->stats.rx_bytes += last->FrameLength;

		// @@@ why is there zero frames at first?
		if (last->FrameLength < 60) {
			dev->stats.runts++;
			continue;
		} else if (last->FrameLength > 1518) {
			dev->stats.oversized++;
			continue;
		}

		pkt = pkt_alloc(last->FrameLength);
		if (pkt == NULL) {
			printf("pkt_alloc failed\r\n");
		} else {
			// @@@ use pkt_add
			memcpy(	pkt->data,
				(char *)rx_desc[last->BufferIndex].RxBufAdr,
				last->FrameLength);

			pkt->len = last->FrameLength;
			en_eth_rx(dev, pkt);
		}

		// Handle wraparound
		if (++last > &rx_sts[RX_STS_NUM - 1])
			last = rx_sts;
	} while (last != cur && q < RX_STS_NUM);

	// @@@ max 255 assignment
	if (q > 0) {
		outl(RXStsEnq, q);
		outl(RXDEnq, q);
	}
}

static void eth_isr(void)
{
	int status;

	// Dispatch interrupt
	status = inl(IntStsC);
	if (status & IntSts_RxSQ)
		process_rx_queue(&eth_if);
	if (status & IntSts_TxSQ)
		process_tx_queue(&eth_if);
}

// Main Ethernet system task
void eth_task_func(void)
{
	int last_link = 0;
	int link;
	int val = 0;

	eth_if.ops->open(&eth_if);

	while (1) {
		val = mii_read(MII_STATUS);
		link = val & MII_STATUS_LINK;

		// Did it come up?
		if (link && !last_link) {
			printf("Ethernet link UP %x\r\n", val);
			last_link = link;
		} else if (!link && last_link) {
			printf("Ethernet link DOWN %x\r\n", val);
			last_link = link;
		}

		sleep(2000);
	}
}

static struct en_eth_ops eth_ops = {
    .xmit = ep9301_xmit,
    .open = eth_reset,
    .release = NULL
};

void ep9301_eth_init(void)
{
	int i;
	struct ip_desc *ipd;
	struct ip_addr ip_addr;
	struct en_ip_route *rt;

	// Obtain necessary memory
	rx_buf = smalloc(RX_DESC_NUM * RX_BUF_SIZE);
	if (rx_buf == NULL) {
		printf("Failed to allocate memory for Ethernet receive buffers\r\n");
		return;
	}

	tx_buf = smalloc(TX_DESC_NUM * TX_BUF_SIZE);
	if (tx_buf == NULL) {
		printf("Failed to allocate memory for Ethernet transmit buffers\r\n");
		return;
	}

	memset(tx_desc, 0, sizeof(struct tx_desc) * TX_DESC_NUM);
	for (i = 0; i < TX_DESC_NUM; ++i) {
		tx_desc[i].TxBufAdr = (uint32_t)tx_buf + (TX_BUF_SIZE * i);
		tx_desc[i].BufferIndex = i;
	}

	// Initialize receive descriptors and status queue
	memset(rx_desc, 0, sizeof(struct rx_desc) * RX_DESC_NUM);
	for (i = 0; i < RX_DESC_NUM; ++i) {
		rx_desc[i].RxBufAdr = (uint32_t)rx_buf + (RX_BUF_SIZE * i);
		rx_desc[i].BufferIndex = i;
		rx_desc[i].BufferLength = RX_BUF_SIZE;
	}

	// Initialize device
	eth_if.irq = INT_MAC;
	strcpy(eth_if.name, "ep9301");
	if (en_eth_register_if(&eth_if)) {
		printf("ep9301_eth_init: failed to register device\r\n");
		return;
	} else {
		printf("registered device at %x\r\n", &eth_if);
	}
	memcpy(&eth_if.addr, (char *)IndAd, 6);
	eth_if.ops = &eth_ops;

	// Bind the device
	eth_if.bound = EN_ETH_BOUND_NET;

	// Set up IP address
	ipd = malloc(sizeof(struct ip_desc));
	if (ipd == NULL) {
		printf("Failed to allocated memory for ip_desc\r\n");
		return;
	}
	memset(ipd, 0, sizeof(struct ip_desc));
	memset(&ip_addr, 0, sizeof(ip_addr));

	ip_addr.addr = (192 << 24) | (168 << 16) | 0 | (99);
	ipd->addr = ip_addr;
	list_add_after(&eth_if.ip_addrs, ipd);

	// Set up route to local network
	rt = malloc(sizeof(struct en_ip_route));
	if (rt == NULL) {
		printf("Failed to allocated memory for ip route\r\n");
		return;
	}

	rt->dst = (192 << 24) | (168 << 16);
	rt->netmask = 0xFFFFFF00;
	rt->gw = 0;	// Direct connection
	rt->flags = IP_ROUTE_FLAG_UP;
	rt->metric = 1;
	rt->eth_if = &eth_if;
	en_ip_route_add(rt);

	// Good to go, fire up the tasks
	eth_task = spawn(eth_task_func, "[eth_ep9301]", PROC_SYSTEM);
	if (eth_task == NULL)
		printf("Failed to spawn Ethernet task\r\n");
}
