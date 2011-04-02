/*
 * arch/init.c
 *
 * Architecture-specific initialization is done in this file, such as
 * interrupts and the timer.
 */

#include "../config.h"

#include <sys/uart.h>
#include <sys/irq.h>
#include <sys/timers.h>

#include <types.h>
#include <string.h>
#include <nand.h>

#include "regs.h"

#define LDR_PC_PC 	0xE59FF000U	// ldr	pc, [pc, ...]

#ifdef ENABLE_MMU
#	include "mmu.h"
#	define VECTOR_BASE	0xFFE00000U
#else
#	define VECTOR_BASE	0x0
#endif

// arch/cpu.s
void arm_da_entry(void);
void arm_und_entry(void);
void arm_pa_entry(void);
void arm_reset_entry(void);

// arch/irq.s
void arm_irq_entry(void);
void arm_svc_entry(void);

// @@@ declare elsewhere
void cons_init(struct uart *uart);
extern struct uart_ops ep93xx_uart_ops;

// @@@ static?
struct uart uart1;

void _putchar(char c)
{
	while (inl(UART1Flag) & TXFF);
	outl(UART1Data, c);
}

void _puthexchar(char c)
{
	char x;
	if ((c >> 4) > 9)
		x = (c >> 4) - 10 + 'A';
	else
		x = (c >> 4) + '0';

	_putchar(x);

	if ((c & 0xf) > 9)
		x = (c & 0xf) - 10 + 'A';
	else
		x = (c & 0xf) + '0';

	_putchar(x);
}

void _puts(const char *s)
{
	while (*s != '\0')
		_putchar(*s++);
}

int _printf_x(uint32_t x)
{
	uint8_t c;
	int n = 0;

	// Dummy code to shut GCC up since it doesn't pick us asm calls
	if (0)
		_printf_x(0);

	c = (x & 0xFF000000) >> 24;
	if (c) {
		_puthexchar(c);
		n += 2;
	}

	c = (x & 0x00FF0000) >> 16;
	if (c || n) {
		_puthexchar(c);
		n += 2;
	}

	c = (x & 0x0000FF00) >> 8;
	if (c || n) {
		_puthexchar(c);
		n += 2;
	}

	c = x & 0xFF;
	if (c || n) {
		_puthexchar(c);
		n += 2;
	}

	if (!n) {
		_putchar('0');
		++n;
	}

	return n;
}

#define HEARTBEAT_MS 100

// Processor-specific timer interrupt
void _timer_int(void)
{
	static volatile uint8_t *leds = (uint8_t *)LEDS;
	static int hb;

	// Clear the interrupt
	outl(Timer3Clear, 0);

	// Call the main kernel timer interrupt
	timer_int();

	// Toggle heartbeat?
	hb += 10;
	if (hb >= HEARTBEAT_MS) {
		hb = 0;
		*leds ^= LED_GREEN;
	}
}

static void ts72xx_init(void)
{
	// Perform TS-72xx specific initialization

#ifdef ENABLE_MMU
	// Map in system-only hardware resources
	_mmu_remap(	(uint32_t *)0x23C00000,			// WDT_FEED
			(uint32_t *)0x23C00000,
			MMU_AP_SRW_UNA);

	_mmu_remap(	(uint32_t *)0x23800000,			// WDT_CTRL
			(uint32_t *)0x23800000,
			MMU_AP_SRW_UNA);
#endif

#ifdef TS_7250
	nand_init();
#endif
}

static void init_interrupts(void)
{
	uint32_t x;
	int i;

	// Disable all interrupts
	outl(VIC1IntSelect, 0);
	outl(VIC1IntEnClear, 0xFFFFFFFFU);
	outl(VIC1ITCR, 0);
	outl(VIC1SoftIntClear, 0xFFFFFFFFU);

	outl(VIC2IntSelect, 0);
	outl(VIC2IntEnClear, 0xFFFFFFFFU);
	outl(VIC2ITCR, 0);
	outl(VIC2SoftIntClear, 0xFFFFFFFFU);

	// Clear any pending interrupts
	for (i = 0; i < 32; ++i) {
		x = inl(VIC1VectAddr);
		outl(VIC1VectAddr, 0);

		x = inl(VIC2VectAddr);
		outl(VIC2VectAddr, 0);
	}

	// Disable all vectored interrupts
	for (i = 0; i < 16; ++i) {
		outl(VIC1VectCntl0 + (i * 4), 0);
		outl(VIC1VectAddr0 + (i * 4), 0);

		outl(VIC2VectCntl0 + (i * 4), 0);
		outl(VIC2VectAddr0 + (i * 4), 0);
	}

	// Set the default vector to be IRQ_TRAP
	// @@@ is this correct?
	outl(VIC1VectAddr, arm_irq_entry);
	outl(VIC2VectAddr, arm_irq_entry);
	outl(VIC1DefVectAddr, arm_irq_entry);
	outl(VIC2DefVectAddr, arm_irq_entry);

	// Enable Timer3 to 100Hz
	// @@@ this should be Timer1 or Timer2, since they are only 16 bit
	// and 16 bit is more than we need
	register_irq_handler(TC3OI, _timer_int, 0);
	enable_irq(TC3OI);

	outl(Timer3Load, 5080);		// 100Hz
	outl(Timer3Control, 0xc8);	// enable timer
}

static void init_traps(void)
{
	volatile uint32_t *vec = (uint32_t *)VECTOR_BASE;

	// Populate the primary vector table with instructions to
	// jump to the corresponding address specified in the secondary table
	vec[0] = LDR_PC_PC | 0x18;	// Reset
	vec[1] = LDR_PC_PC | 0x18;	// Undefined instruction
	vec[2] = LDR_PC_PC | 0x18;	// Software interrupt
	vec[3] = LDR_PC_PC | 0x18;	// Prefetch abort
	vec[4] = LDR_PC_PC | 0x18;	// Data abort
	vec[5] = 0;			// Reserved
	vec[6] = LDR_PC_PC | 0x18;	// IRQ
	vec[7] = LDR_PC_PC | 0x18;	// FIQ

	// Populate the secondary vector table with our trap handlers
	vec[8 + 0] = (uint32_t)arm_reset_entry;	// Reset
	vec[8 + 1] = (uint32_t)arm_und_entry;	// Undefined instruction
	vec[8 + 2] = (uint32_t)arm_svc_entry;	// Software interrupt
	vec[8 + 3] = (uint32_t)arm_pa_entry;	// Prefetch abort
	vec[8 + 4] = (uint32_t)arm_da_entry;	// Data abort
	vec[8 + 5] = 0;				// Reserved
	vec[8 + 6] = (uint32_t)arm_irq_entry;	// IRQ
	//vec[8 + 7] = (uint32_t)trap;		// FIQ @@@
}

// Perform low-level init here such as clock, interrupts, console
void arch_init(void)
{
	init_interrupts();
	init_traps();

	ts72xx_init();

	// Initialize UART1/console
	memset(&uart1, 0, sizeof(struct uart));
	uart1.uart_ops = &ep93xx_uart_ops;
	uart1.uart_ops->open(&uart1);
	cons_init(&uart1);
}
