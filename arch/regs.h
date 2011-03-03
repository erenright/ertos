/*
 * arch/regs.h
 *
 * Defines EP93xx system registers, as well as IRQ definitions and inl() and outl() accessors.
 */
#ifndef _ARCH_REGS_H
#define _ARCH_REGS_H

#define INT_ENABLE		(1 << 5)

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


// Memory accessors
#define inl(addr) (*((volatile uint32_t *)addr))
#define outl(addr, val) (*((volatile uint32_t *)(addr))) = (uint32_t)val

#define inw(addr) (*((volatile uint16_t *)addr))
#define outw(addr, val) (*((volatile uint16_t *)(addr))) = (uint16_t)val

#endif // !_ARCH_REGS_H
