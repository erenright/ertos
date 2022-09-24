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
 * arch/regs.h
 *
 * Defines EP93xx system registers, as well as IRQ definitions and inl() and outl() accessors.
 */
#ifndef _ARCH_REGS_H
#define _ARCH_REGS_H

#define INT_ENABLE		(1 << 5)

#define NAND_DATA		0x60000000		// NAND Flash data

#define NAND_CTRL		0x60400000		// NAND Flash control
#define NAND_CTRL_ALE		(1)			// Address latch enable
#define NAND_CTRL_CLE		(1<<1)			// Command latch enable
#define NAND_CTRL_NCE		(1<<2)			// Chip enable
#define NAND_CTRL_MASK		0x03

#define NAND_BUSY		0x60800000		// NAND Busy status
#define NAND_BUSY_BIT		(1<<5)


#define REG_BASE		0x80000000

#define VIC1_BASE		(REG_BASE + 0x000B0000)
#define VIC1IntSelect		(VIC1_BASE + 0x000C)	// Interrupt enable register
#define VIC1IntEnable		(VIC1_BASE + 0x0010)	// Interrupt enable register
#define VIC1IntEnClear		(VIC1_BASE + 0x0014)	// Interrupt enable clear
#define VIC1SoftIntClear	(VIC1_BASE + 0x0014)	// Software interrupt clear
#define VIC1Protection		(VIC1_BASE + 0x0020)	// Protection enable register
#define VIC1VectAddr		(VIC1_BASE + 0x0030)	// Vector address register
#define VIC1DefVectAddr		(VIC1_BASE + 0x0034)	// Default vector address register
#define VIC1VectAddr0		(VIC1_BASE + 0x0100)	// Vector address 0 register
#define VIC1VectCntl0		(VIC1_BASE + 0x0200)	// Vector control 0 register
#define VIC1ITCR		(VIC1_BASE + 0x0300)	// Test control register

#define VIC2_BASE		(REG_BASE + 0x000C0000)
#define VIC2IntSelect		(VIC2_BASE + 0x000C)	// Interrupt enable register
#define VIC2IntEnable		(VIC2_BASE + 0x0010)	// Interrupt enable register
#define VIC2IntEnClear		(VIC2_BASE + 0x0014)	// Interrupt enable clear
#define VIC2SoftIntClear	(VIC2_BASE + 0x0014)	// Software interrupt clear
#define VIC2Protection		(VIC2_BASE + 0x0020)	// Protection enable register
#define VIC2VectAddr		(VIC2_BASE + 0x0030)	// Vector address register
#define VIC2DefVectAddr		(VIC2_BASE + 0x0034)	// Default vector address register
#define VIC2VectAddr0		(VIC2_BASE + 0x0100)	// Vector address 0 register
#define VIC2VectAddr1		(VIC2_BASE + 0x0104)	// Vector address 1 register
#define VIC2VectCntl0		(VIC2_BASE + 0x0200)	// Vector control 0 register
#define VIC2VectCntl1		(VIC2_BASE + 0x0204)	// Vector control 1 register
#define VIC2ITCR		(VIC2_BASE + 0x0300)	// Test control register

#define TIMER_BASE		(REG_BASE + 0x00810000)
#define	Timer3Load		(TIMER_BASE + 0x0080)
#define	Timer3Value		(TIMER_BASE + 0x0084)
#define	Timer3Control		(TIMER_BASE + 0x0088)
#define	Timer3Clear		(TIMER_BASE + 0x008C)

#define GPIO_BASE		(REG_BASE + 0x00840000)
#define	PEDR			(GPIO_BASE + 0x0020)
#define	PEDDR			(GPIO_BASE + 0x0024)

#define LEDS			PEDR
#define LED_GREEN		0x01
#define LED_RED			0x02

#define SPI_BASE		(REG_BASE + 0x008A0000)

#define SSPCR0			(SPI_BASE + 0x0000)	// Control register 0
#define SSPCR0_SCR_MASK		(0xFF00)		// Serial clock rate
#define SSPCR0_SCR_SHIFT	(1<<8)			// Serial clock rate
#define SSPCR0_SPH		(1<<7)			// SCLKOUT phase
#define SSPCR0_SPO		(1<<6)			// SCLKOUT polarity
#define SSPCR0_FRF_MASK		(0x30)			// Frame format
#define SSPCR0_FRF_SHIFT	(1<<4)
#define SSPCR0_FRF_SPI		(0)			// Motorola SPI
#define SSPCR0_DSS_MASK		(0x0F)
#define SSPCR0_DSS_SHIFT	(0)
#define SSPCR0_DSS_8_BIT	(7)

#define SSPCR1			(SPI_BASE + 0x0004)	// Control register 1
#define SSPCR1_SSE		(1<<4)			// Sync. serial enable
#define SSPCR1_LBM		(1<<3)			// Loopback mode
#define SSPDR			(SPI_BASE + 0x0008)	// RX/TX FIFO

#define SSPSR			(SPI_BASE + 0x000C)	// Status register
#define SSPSR_BSY		(1<<4)			// Busy bit

#define SSPCPSR			(SPI_BASE + 0x0010)	// Clock prescale reg.
#define SSPIIR			(SPI_BASE + 0x0014)	// Interrupt ID register
#define SSPICR			(SPI_BASE + 0x0014)	// Interrupt clear reg.

#define UART_BASE		(REG_BASE + 0x008C0000)
#define UART1Data		(UART_BASE)
#define UART1LinCtrlHigh	(UART_BASE + 0x0008)
#define UART1LinCtrlMed		(UART_BASE + 0x000C)
#define UART1LinCtrlLow		(UART_BASE + 0x0010)
#define UART1Ctrl		(UART_BASE + 0x0014)
#define UART1Flag		(UART_BASE + 0x0018)
#define UART1IntIDIntClr	(UART_BASE + 0x001C)

// UART1LinCtrlHigh bits
#define FEN	0x10	// FIFO enable
#define WLEN_8	0x60	// 8 bits per frame

// UART1Ctrl bits
#define UARTE	0x01	// UART enable
#define RIE	0x10	// RX interrupt enable
#define TIE	0x20	// TX interrupt enable

// UART1Flag bits
#define RXFE	0x10	// RX FIFO empty
#define TXFF	0x20	// TX FIFO full
#define RXFF	0x40	// RX FIFO full
#define TXFE	0x80	// TX FIFO empty

// UART1IntIDIntClr bits
#define RIS	0x02	// RX interrupt status
#define TIS	0x04	// TX interrupt status

#define TINTR		35	// 64Hz timer interrupt
#define INT_MAC		39	// Ethernet MAC Interrupt
#define TC3OI		51	// Timer3 interrupt
#define INT_UART1	52	// UART1 interrupt
#define USHINTR		56	// USB Host Interrupt

// Ethernet Controller Registers
#define MACBase		(REG_BASE + 0x00010000)

#define RXCtl		(MACBase + 0x0000)	// Receiver Control
#define RXCtl_SRxON	(1<<16)			// Serial Receive ON
#define RXCtl_RCRCA	(1<<13)			// Runt CRCA
#define RXCtl_PA	(1<<11)			// Promiscuous A
#define RXCtl_BA	(1<<10)			// Broadcast A
#define RXCtl_IA0	(1)			// Individual Address 0

#define TXCtl		(MACBase + 0x0004)	// Transmitter Control
#define TXCtl_STxON	(1)			// Serial Transmit ON

#define TestCtl		(MACBase + 0x0008)	// Test Control

#define MIICmd		(MACBase + 0x0010)	// MII Command
#define MIICmd_Write	(1<<14)
#define MIICmd_Read	(1<<15)
#define MIICmd_REGAD	0x001F			// REGAD mask
#define MIICmd_PHYAD	0x03E0			// PHYAD mask

#define MIIData		(MACBase + 0x0014)	// MII Data

#define MIISts		(MACBase + 0x0018)	// MII Status
#define MIISts_Busy	(1)

#define SelfCtl		(MACBase + 0x0020)	// Self Control
#define SelfCtl_MDCDIV_SHIFT	9
#define SelfCtl_PSPRS	(1<<8)			// Preamble Suppress
#define SelfCtl_RESET	(1)

#define IntEn		(MACBase + 0x0024)	// Interrupt Enable
#define IntEn_PHYSIE	(1<<11)			// PHY status interrupt
#define IntEn_REOFIE	(1<<2)			// RX end-of-frame
#define IntEn_REOBIE	(1<<1)			// RX end-of-buffer
#define IntEn_RHDRIE	(1)			// RX receive header status

#define IntStsP		(MACBase + 0x0028)	// Interrupt Status Preserve
#define IntStsC		(MACBase + 0x002C)	// Interrupt Status Clear
#define IntSts_TxSQ	(1<<3)			// Transmit status queue post
#define IntSts_RxSQ	(1<<2)			// Receive status queue post

#define IndAd		(MACBase + 0x0050)	// Individual Address (MAC addr)

#define GIntMsk		(MACBase + 0x0064)	// Global Interrupt Mask
#define GIntMsk_INT	(1<<15)			// Mask interrupts

#define BMCtl		(MACBase + 0x0080)	// Bus Master Control
#define BMCtl_RxEn	(1)			// Receive Enable
#define BMCtl_TxEn	(1<<8)			// Transmit Enable

#define BMSts		(MACBase + 0x0084)	// Bus Master Status
#define BMSts_RxAct	(1<<3)			// Receive Active

#define RXDQBAdd	(MACBase + 0x0090)	// RX Descriptor Queue Base Addr
#define RXDQBLen	(MACBase + 0x0094)	// RX Descriptor Queue Base Len
#define RXDQCurLen	(MACBase + 0x0096)	// RX Descriptor Queue Current Len
#define RXDCurAdd	(MACBase + 0x0098)	// RX Descriptor Current Addr
#define RXDEnq		(MACBase + 0x009C)	// RX Descriptor Enqueue

#define RXStsQBAdd	(MACBase + 0x00A0)	// RX Status Queue Base Addr
#define RXStsQBLen	(MACBase + 0x00A4)	// RX Status Queue Base Len
#define RXStsQCurLen	(MACBase + 0x00A6)	// RX Status Queue Current Len
#define RXStsQCurAdd	(MACBase + 0x00A8)	// RX Status Queue Current Addr
#define RXStsEnq	(MACBase + 0x00AC)	// RX Status Queue Enqueue

#define TXDQBAdd	(MACBase + 0x00B0)	// TX Descriptor Queue Base Addr
#define TXDQBLen	(MACBase + 0x00B4)	// TX Descriptor Queue Base Len
#define TXDQCurLen	(MACBase + 0x00B6)	// TX Descriptor Queue Current Len
#define TXDQCurAdd	(MACBase + 0x00B8)	// TX Descriptor Current Addr
#define TXDEnq		(MACBase + 0x00BC)	// TX Descriptor Enqueue
#define TXStsQBAdd	(MACBase + 0x00C0)	// TX Status Queue Base Addr
#define TXStsQBLen	(MACBase + 0x00C4)	// TX Status Queue Base Len
#define TXStsQCurLen	(MACBase + 0x00C6)	// TX Status Queue Current Len
#define TXStsQCurAdd	(MACBase + 0x00C8)	// TX Status Queue Current Addr

// USB / OHCI Registers

#define	OHCI_BASE	(REG_BASE + 0x00020000)

#define	HcRevision		(OHCI_BASE + 0x0000)

#define	HcControl		(OHCI_BASE + 0x0004)
#define HcControl_HCFS_MASK	((1<<7)|(1<<6))
#define HcControl_USBRESET	(0)
#define HcControl_USBRESUME	(1<<6)
#define HcControl_USBOPERATIONAL (2<<6)
#define HcControl_USBSUSPEND	(3<<6)
#define HcControl_IR		(1<<8)

#define HcCommandStatus		(OHCI_BASE + 0x0008)
#define HcCommandStatus_HCR	(1)

#define HcInterruptStatus	(OHCI_BASE + 0x000C)

#define HcInterruptEnable	(OHCI_BASE + 0x0010)
#define HcInterruptEnable_SF	(1<<2)	// Start of Frame
#define HcInterruptEnable_RHSC	(1<<6)	// Root hub status change
#define HcInterruptEnable_OC	(1<<30)	// Ownership change
#define HcInterruptEnable_MIE	(1<<31)	// Master interrupt enable

#define HcInterruptDisable	(OHCI_BASE + 0x0014)
#define HcHCCA			(OHCI_BASE + 0x0018)
#define HcPeriodCurrentED	(OHCI_BASE + 0x001C)
#define HcControlHeadED		(OHCI_BASE + 0x0020)
#define HcControlCurrentED	(OHCI_BASE + 0x0024)
#define HcBulkHeadED		(OHCI_BASE + 0x0028)
#define HcBulkCurrentED		(OHCI_BASE + 0x002C)
#define HcDoneHead		(OHCI_BASE + 0x0030)

#define HcFmInterval		(OHCI_BASE + 0x0034)
#define HcFmInterval_FIT	(1<<31)

#define HcFmRemaining		(OHCI_BASE + 0x0038)
#define HcFmNumber		(OHCI_BASE + 0x003C)
#define HcPeriodicStart		(OHCI_BASE + 0x0040)
#define HcLSThreshold		(OHCI_BASE + 0x0044)
#define HcRhDescriptorA		(OHCI_BASE + 0x0048)
#define HcRhDescriptorB		(OHCI_BASE + 0x004C)
#define HcRhStatus		(OHCI_BASE + 0x0050)

#define HcRhPortStatus1		(OHCI_BASE + 0x0054)
#define HcRhPortStatus2		(OHCI_BASE + 0x005C)
#define HcRhPortStatus_CCS	(1)	// Current Connect Status
#define HcRhPortStatus_CSC	(1<<16)	// Connect Status Change

// ep9301-specific
#define USBCfgCtrl		(OHCI_BASE + 0x0080)
#define USBHCISts		(OHCI_BASE + 0x0084)

// GPIO Registers

#define GPIO_BASE	(REG_BASE + 0x00840000)

#define PADR		(GPIO_BASE + 0x0000)	// Port A Data
#define PBDR		(GPIO_BASE + 0x0004)	// Port B Data
#define PCDR		(GPIO_BASE + 0x0008)	// Port C Data
#define	PADDR		(GPIO_BASE + 0x0010)	// Port A Data Direction
#define	PBDDR		(GPIO_BASE + 0x0014)	// Port B Data Direction
#define	PCDDR		(GPIO_BASE + 0x0018)	// Port C Data Direction
#define PFDR		(GPIO_BASE + 0x0030)	// Port F Data
#define PFDDR		(GPIO_BASE + 0x0034)	// Port F Data

// Internal registers

#define	INTERNAL_BASE	(REG_BASE + 0x00930000)

#define	PwrCnt		(INTERNAL_BASE + 0x0004)// Clock/debug control status
#define PwrCnt_USH_EN	(1<<28)			// USB host clock


// Memory accessors
#define inl(addr) (*((volatile uint32_t *)addr))
#define outl(addr, val) (*((volatile uint32_t *)(addr))) = (uint32_t)val

#define inw(addr) (*((volatile uint16_t *)addr))
#define outw(addr, val) (*((volatile uint16_t *)(addr))) = (uint16_t)val

#define inb(addr) (*((volatile uint8_t *)addr))
#define outb(addr, val) (*((volatile uint8_t *)(addr))) = (uint8_t)val

#endif // !_ARCH_REGS_H
