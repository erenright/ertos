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
#define TC3OI		51	// Timer3 interrupt
#define INT_UART1	52	// UART1 interrupt

#define inl(addr) (*((volatile uint32_t *)addr))
#define outl(addr, val) (*((volatile uint32_t *)(addr))) = (uint32_t)val

#endif // !_ARCH_REGS_H
