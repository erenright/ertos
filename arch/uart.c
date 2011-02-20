/*
 * uart.c
 *
 * EP93xx UART device driver.
 */

#include <sys/uart.h>
#include <sys/irq.h>
#include <sys/list.h>

#include <string.h>

#include "regs.h"

static void ep93xx_uart_enable_rx(struct uart *uart);

extern struct uart uart1; // @@@

static void uart1_intr(void)
{
	uint32_t reg = inl(UART1IntIDIntClr);

	// RX interrupt?
	if (reg & RIS)
		uart1.uart_ops->rx(&uart1);

	// TX interrupt?
	if (reg & TIS)
		uart1.uart_ops->tx(&uart1);
}

// Returns zero on success, -1 on failure
static int ep93xx_uart_open(struct uart *uart)
{
	uint32_t br;

	// Allocate a wait queue for this device
	// @@@ need a generic completion creation function
	uart->wait.wait = bfifo_alloc(10);
	if (uart->wait.wait == NULL)
		return -1;

	// Clear FIFOs
	memset(&uart->rx_fifo, 0, sizeof(struct uart_fifo));
	memset(&uart->tx_fifo, 0, sizeof(struct uart_fifo));

	uart->baud = 115200;	// Default to 115200

	// BAUD rate divisor = FUARTCLK / (16 * desired_rate)
	br = 3; // 7372800 / (16 * uart->baud) - 1;
	outl(UART1LinCtrlLow, br & 0xff);
	outl(UART1LinCtrlMed, (br & 0xff00) >> 8);

	// Enable FIFOs, 8N1
	//*UART1LinCtrlHigh = FEN | WLEN_8;// High must be last to cause Med
					// Low to be registered
	// 8N1
	outl(UART1LinCtrlHigh, WLEN_8);

	// Set up a single interrupt
	register_irq_handler(INT_UART1, uart1_intr);

	// Enable the UART, with TX and RX interrupts
	outl(UART1Ctrl, RIE | TIE | UARTE);
	enable_irq(INT_UART1);

	//ep93xx_uart_enable_rx(uart);

	uart->state = UART_OPEN;

	return 0;
}

static void ep93xx_uart_close(struct uart *uart)
{
	// Disable the UART
	outl(UART1Ctrl, (inl(UART1Ctrl) &= ~UARTE));
	
	// Clear FIFOs
	memset(&uart->rx_fifo, 0, sizeof(struct uart_fifo));
	memset(&uart->tx_fifo, 0, sizeof(struct uart_fifo));

	uart->state = UART_CLOSED;
}

// Return 0 on success, -1 on failure
static int ep93xx_uart_set_baud(struct uart *uart, int baud)
{
	// @@@ Actually set the BAUD!
	uart->baud = baud;

	return 0;
}

static void ep93xx_uart_disable_tx(struct uart *uart)
{
	// Disable the TX interrupt
	outl(UART1Ctrl, (inl(UART1Ctrl) &= ~TIE));
}

static void ep93xx_uart_enable_tx(struct uart *uart)
{
	// Enable the TX interrupt
	outl(UART1Ctrl, (inl(UART1Ctrl) |= TIE));
}

static void ep93xx_uart_disable_rx(struct uart *uart)
{
	// Disable the RX interrupt
	outl(UART1Ctrl, (inl(UART1Ctrl) &= ~RIE));
}

static void ep93xx_uart_enable_rx(struct uart *uart)
{
	// Enable the RX interrupt
	outl(UART1Ctrl, (inl(UART1Ctrl) |= RIE));
}

// kernel/syscall.c @@@
int sys_wake(uint32_t *args);

static void ep93xx_uart_rx(struct uart *uart)
{
	struct uart_fifo *f = &uart->rx_fifo;
	struct completion *c = &uart->wait;
	int free = uart_fifo_free(f);

	while (!(inl(UART1Flag) & RXFE) && free > 0) {
		f->data[f->tail] = inl(UART1Data);
		if (++f->tail >= sizeof(f->data))
			f->tail = 0;
		--free;
	}

	// Notify anyone waiting @@@ must break system call up
	sys_wake((uint32_t *)&c);
}

static void ep93xx_uart_tx(struct uart *uart)
{
	struct uart_fifo *f = &uart->tx_fifo;

	// Disable transmit if we have no data
	if (f->head == f->tail) {
		uart->uart_ops->disable_tx(uart);
		return;
	}

	// TX interrupt called us, so fill up the queue as much as possible
	while (!(inl(UART1Flag) & TXFF) && f->head != f->tail) {
		outl(UART1Data, f->data[f->head]);
		if (++f->head >= sizeof(f->data))
			f->head = 0;
	}
}

struct uart_ops ep93xx_uart_ops = {
	.open		= ep93xx_uart_open,
	.close		= ep93xx_uart_close,
	.set_baud	= ep93xx_uart_set_baud,
	.disable_tx	= ep93xx_uart_disable_tx,
	.enable_tx	= ep93xx_uart_enable_tx,
	.disable_rx	= ep93xx_uart_disable_rx,
	.enable_rx	= ep93xx_uart_enable_rx,
	.rx		= ep93xx_uart_rx,
	.tx		= ep93xx_uart_tx,
};
