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
 * include/sys/mii.h
 *
 * Media Independant Interface definitions.
 */

#ifndef _SYS_MII_H
#define _SYS_MII_H

// MII management register set, per IEEE 802.3 table 22-6.
#define MII_CONTROL		0	// Control
#define MII_STATUS		1	// Status
#define MII_PHYID1		2	// PHY Identifier 1
#define MII_PHYID2		3	// PHY Identifier 2
#define MII_AUTONG_ADV		4	// Auto-Negotiation Advertisement
#define MII_AUTONG_LPBPA	5	// Auto-Negotiation Link Partner Base
					//	Page Ability
#define MII_AUTONG_EXP		6	// Auto-Negotiation Expansion
#define MII_AUTONG_NPT		7	// Auto-Negotiation Next Page Transmit
#define MII_AUTONG_LPRNP	8	// Auto-Negotiation Link Partner
					//	Received Next Page
#define MII_MS_CONTROL		9	// MASTER-SLAVE Control
#define MII_MS_STATUS		10	// MASTER-SLAVE Status
#define MII_PSE_CONTROL		11	// PSE Control
#define MII_PSE_STATUS		12	// PSE Status
#define MII_MMD_ACC_CONTROL	13	// MMD Access Control
#define MII_MMD_ACC_DATA	14	// MMD Access Address Data Register
#define MII_XSTATUS		15	// Extended Status

// MII_CONTROL bit definitions
#define MII_CONTROL_RESET	(1<<15)	// Reset
#define MII_CONTROL_LOOPBACK	(1<<14)	// Loopback

#define MII_CONTROL_SPEED	((1<<13) | (1<<6))	// Speed Selection
#define MII_CONTROL_SPEED_1000	(1<<6)	// 1000 Mb/s
#define MII_CONTROL_SPEED_100	(1<<13)	// 100 Mb/s
#define MII_CONTROL_SPEED_10	(0)	// 10 Mb/s

#define MII_CONTROL_AUTONGEN	(1<<12)	// Auto-Negotiation Enable
#define MII_CONTROL_POWERDOWN	(1<<11)	// Power Down
#define MII_CONTROL_ISOLATE	(1<<10)	// Isolate
#define MII_CONTROL_RESTART_AUTONG	(1<<9)	// Restart Auto-Negotiation

#define MII_CONTROL_DUPLEX	(1<<8)	// Duplex Mode
#define MII_CONTROL_DUPLEX_FULL	(1)	// Full duplex
#define MII_CONTROL_DUPLEX_HALF	(0)	// Half duplex

#define MII_CONTROL_UNIDIR	(1<<5)	// Unidirectional enable

// MII_STATUS bit definitions
#define MII_STATUS_AUTONG	(1<<5)	// Auto-Negotiation Complete
#define MII_STATUS_CAN_AUTONG	(1<<3)	// Auto-Negotiation Ability
#define MII_STATUS_LINK		(1<<2)	// Link Status

#endif /* !_SYS_MII_H */
