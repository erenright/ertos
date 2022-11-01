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
 * user/console.c
 *
 * Console with user commands.  This task displays a ticker on the console,
 * waiting for the first user keystroke.  Upon receipt of the keystroke,
 * it drops to a command prompt.
 */

#include "../config.h"

#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/kernel.h>

#include <stdio.h>
#include <sleep.h>
#include <cons.h>
#include <string.h>
#include <kstat.h>

#ifdef CONFIG_NAND
#	include <nand.h>
#endif

#ifdef CONFIG_SPI
#	include <spi.h>
#endif

#ifdef CONFIG_USB
#	include <ohci.h>
#	include <usb.h>
#endif

#include "../arch/regs.h"

#ifdef CONFIG_NET
#	include "../net/dll/arp.h"
#endif

// Maximum number of arguments a command may have
#define MAX_ARGS 8

//#define _DBG

static inline int isspace(int c)
{
	if (c == ' ')
		return 1;
	if (c == '\t')
		return 1;
	if (c == '\r')
		return 1;
	if (c == '\n')
		return 1;
	if (c == '\f')
		return 1;
	if (c == '\v')
		return 1;

	return 0;
}

static void cmd_ps(int argc, char *argv[])
{
	struct proc *p;
	struct proc *op;

	printf("PID\tState\tName\r\n");

	// @@@ dangerous, no locking
	p = cur;
	op = p;

	do {
		printf("0x%x\t", p->pid);

		switch (p->state) {
		case PROC_ACTIVE:
			printf("ACTIVE\t");
			break;

		case PROC_RUN:
			printf("RUN\t");
			break;

		case PROC_SLEEP:
			printf("SLEEP\t");
			break;

		case PROC_KILLED:
			printf("KILLED\t");
			break;

		default:
			printf("UNKNOWN\t");
			break;
		}

		printf("%s\r\n", p->name);

		p = (struct proc *)p->list.next;
	} while (p != op);
}

static int ticker_done = 0;

static void cmd_ticker(int argc, char *argv[])
{
	static char ticker[] = { '/' , '-', '\\', '|' };	
	int tick = 0;
	int len = 0;
	char buf[128];

	ticker_done = 0;

	while (!ticker_done) {
		sleep(250);

		putchar('\r');
		putchar(ticker[tick]);
		if (++tick >= sizeof(ticker))
			tick = 0;

		// Data read?
		len = cons_read(buf, sizeof(buf));
		if (len > 0) {
			ticker_done = 1;// Yes, break to command prompt
		}
	}

	putchar('\r');
}

static void cmd_exit(int argc, char *argv[])
{
	ticker_done = 0;
}

static void cmd_dumpmem(int argc, char *argv[])
{
	//int i;
	int len;
	int addr;
	int x;

	if (argc != 3) {
		puts("dumpmem <addr> <len>");
		return;
	}

	addr = atoi(argv[1]);
	len = atoi(argv[2]);
	if (len == 0)
		len = 16;

	printf("dumping 0x%x+0x%x\r\n\r\n", addr, len);

	x = 0;
	while (len-- > 0) {
		if (x == 0)
			printf("%x| ", addr);
		else if (x == 4)
			printf("   ");

		printf("%x ", *(char *)addr);

		if (++x >= 8) {
			x = 0;
			puts("");
		}

		++addr;
	}

	puts("");
}

void handle_alarm(void)
{
	printf("alarm hit!\r\n");
}

static void cmd_alarm(int argc, char *argv[])
{
	struct alarm a;

	if (argc != 3) {
		printf("alarm <msec> <oneshot>\r\n");
		return;
	}

	a.msec = atoi(argv[1]);
	a.oneshot = atoi(argv[2]);
	a.handler = handle_alarm;

	alarm(&a);

	printf("alarm armed\r\n");
}

static void cmd_reset(int argc, char *argv[])
{
	reset();
	/*NOTREACHED*/

	printf("reset failed!?\r\n");
}

static void cmd_kstat(int argc, char *arg[])
{
	struct kstat lkstat;
	int rc;

	rc = kstat_get(&lkstat);
	if (rc != 9) {	// @@@ FIXME can not detect correct retval
		printf("kstat_get: %d\r\n", rc);
	} else {
		printf("ISR recursions prevented: %d\r\n",lkstat.isr_recursion);
	}
}

#ifdef CONFIG_NET
static void cmd_netstat(int argc, char *argv[])
{
	struct netstat netstat;
	int rc;

	rc = netstat_get(&netstat);
	if (rc != 10) { // @@@ FIXME can not detect correct retval
		printf("netstat_get: %d\r\n", rc);
	} else {
		//if (netstat.eth.name[0] != '\0') {
			puts(netstat.eth.name);
			printf("\tBytes received:  %d\r\n", netstat.eth.stats.rx_bytes);
			printf("\tBytes sent:      %d\r\n", netstat.eth.stats.tx_bytes);
			printf("\tFrames received: %d\r\n", netstat.eth.stats.rx_frames);
			printf("\tFrames sent:     %d\r\n", netstat.eth.stats.tx_frames);
			printf("\tRunts:           %d\r\n", netstat.eth.stats.runts);
			printf("\tOversized:       %d\r\n", netstat.eth.stats.oversized);
			printf("\tFCS errors:      %d\r\n", netstat.eth.stats.fcs_errors);
		//}
	}
}

static void cmd_arp(int argc, char *argv[])
{
	struct list *p;
	struct en_arp_entry *arp;

	printf("MAC\t\t\tIP\r\n");

	for (p = arp_cache_list.next; p != NULL; p = p->next) {
		arp = (struct en_arp_entry *)p;

		printf("%x:%x:%x:%x:%x:%x\t",
			arp->hrd_addr.addr[0],
			arp->hrd_addr.addr[1],
			arp->hrd_addr.addr[2],
			arp->hrd_addr.addr[3],
			arp->hrd_addr.addr[4],
			arp->hrd_addr.addr[5]);

		printf("%d.%d.%d.%d\r\n",
			(arp->proto_addr.addr & 0xFF000000) >> 24,
			(arp->proto_addr.addr & 0x00FF0000) >> 16,
			(arp->proto_addr.addr & 0x0000FF00) >> 8,
			(arp->proto_addr.addr & 0x000000FF));
	}
}

static void cmd_ifconfig(int argc, char *argv[])
{
	struct list *p, *p2;
	struct en_eth_if *eth_if;
	struct ip_desc *ip;

	for (p = eth_if_list.next; p != NULL; p = p->next) {
		eth_if = (struct en_eth_if *)p;

		puts(eth_if->name);

		for (p2 = eth_if->ip_addrs.next; p2 != NULL; p2 = p2->next) {
			ip = (struct ip_desc *)p2;

			printf("\taddress %d.%d.%d.%d\r\n",
				(ip->addr.addr & 0xFF000000) >> 24,
				(ip->addr.addr & 0x00FF0000) >> 16,
				(ip->addr.addr & 0x0000FF00) >> 8,
				(ip->addr.addr & 0x000000FF));
		}
	}
}
#endif // CONFIG_NET

#ifdef CONFIG_NAND
static void cmd_nand_read_page(int page)
{
	char *buf = malloc(NAND_RAW_PAGE_SIZE);
	int i;
	int x = 0;
	int y = 0;
	int c;

	if (buf == NULL) {
		printf("Unable to allocate buffer for read\r\n");
		return;
	}

	nand_read_page(page, buf);

	for (i = 0, x = 0; i < NAND_RAW_PAGE_SIZE; ++i) {
		if (x == 0)
			printf("%x| ", i);
		else if (x == 4)
			printf("   ");

		printf("%x ", buf[i]);

		if (++x >= 8) {
			x = 0;
			puts("");

			if (++y == 23) {
				stdio_buf_disable();
				printf("More...");
				c = getchar();
				stdio_buf_enable();
				puts("");

				y = 0;

				if (c == 'q')
					break;
			}
		}
	}

	if (x != 0)
		puts("");

	free(buf);
}

static void cmd_nand_read(int page, int off, int len)
{
	char *buf = malloc(len);
	int i;
	int x = 0;
	int y = 0;
	int c;

	if (buf == NULL) {
		printf("Unable to allocate buffer for read\r\n");
		return;
	}

	nand_read(page, off, buf, len);

	for (i = 0, x = 0; i < len; ++i) {
		if (x == 0)
			printf("%x| ", off + i);
		else if (x == 4)
			printf("   ");

		printf("%x ", buf[i]);

		if (++x >= 8) {
			x = 0;
			puts("");

			if (++y == 23) {
				stdio_buf_disable();
				printf("More...");
				c = getchar();
				stdio_buf_enable();
				puts("");

				y = 0;

				if (c == 'q')
					break;
			}
		}
	}

	if (x != 0)
		puts("");

	free(buf);
}

static void cmd_nand_bbscan(void)
{
	int i;
	uint32_t bad = 0;
	char status;

	for (i = 0; i < NAND_NUM_PAGES; ++i) {
		nand_read(i, NAND_PAGE_SIZE, &status, 1);

		// Non-FF is a bad page
		if (status != 0xFF) {
			++bad;

			printf("0x%x\r\n");
		}
	}

	printf("%d bad pages found\r\n", bad);
}

static void cmd_nand_eraseblock(int block)
{
	int status;
	int addr = block * NAND_PAGES_PER_BLOCK * NAND_PAGE_SIZE;
	int c;

	// The first 128kB on the TS-7250 128MB is the bootrom
	// and the range 0x07D20000+0x40000 is RedBoot, so warn
	// the user that this could be dangerous
	if (addr < (128 * 1024)	// bootrom
		|| (addr >= 0x07D20000 && addr < 0x07D60000)) { // RedBoot

		stdio_buf_disable();
		printf("This area includes the bootrom or RedBoot. Continue? [y/n] ");
		c = getchar();
		stdio_buf_enable();
		puts("");

		if (c != 'y')
			return;
	}

	status = nand_erase(block);

	printf("Erase status for %d to %d: 0x%x\r\n", 
		addr,
		addr + (NAND_PAGES_PER_BLOCK * NAND_PAGE_SIZE),
		status);
}

static void cmd_nand_fillsect(int page, int sect, int pattern)
{
	char *buf;
	int status, i;

	buf = malloc(512);
	if (buf == NULL) {
		printf("cmd_nand_fillsect: failed to allocate buffer\r\n");
		return;
	}

	if (pattern > 0xFF) {
		for (i = 0; i < 512; ++i)
			buf[i] = i & 0xFF;
	} else {
		memset(buf, pattern & 0xFF, 512);
	}

	status = nand_write_sector(page, sect, buf);

	printf("Program status: 0x%x\r\n", status);

	free(buf);
}

static void cmd_nand(int argc, char *argv[])
{
	if (argc < 2)
		goto out_err;

	if (!strcmp(argv[1], "reset")) {
		nand_reset();
	} else if (!strcmp(argv[1], "readid")) {
		printf("%x\r\n", nand_read_id());
	} else if (!strcmp(argv[1], "readpage")) {
		if (argc != 3)
			goto out_err;

		cmd_nand_read_page(atoi(argv[2]));
	} else if (!strcmp(argv[1], "read")) {
		if (argc != 5)
			goto out_err;

		cmd_nand_read(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	} else if (!strcmp(argv[1], "bbscan")) {
		cmd_nand_bbscan();
	} else if (!strcmp(argv[1], "eraseblock")) {
		if (argc != 3)
			goto out_err;

		cmd_nand_eraseblock(atoi(argv[2]));
	} else if (!strcmp(argv[1], "fillsect")) {
		if (argc != 5)
			goto out_err;

		cmd_nand_fillsect(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
	} else {
		goto out_err;
	}

	return;

out_err:

	printf("supported commands:\r\n");
	printf("\tbbscan\r\n");
	printf("\teraseblock <block>\r\n");
	printf("\tread <page> <offset> <len>\r\n");
	printf("\treadpage <page>\r\n");
	printf("\treset\r\n");
	printf("\treadid\r\n");
	printf("\tfillsect <page> <sect> <pattern>\r\n");
}
#endif // CONFIG_NAND

#ifdef CONFIG_SPI
static void cmd_spi(int argc, char *argv[])
{
	if (argc < 2)
		goto out_err;

	if (!strcmp(argv[1], "loopback")) {
		spi_loopback_test();
	} else {
		goto out_err;
	}

	return;

out_err:

	printf("supported commands:\r\n");
	printf("\tloopback\r\n");
}

static void cmd_hmm_cs(int chip, int on)
{
	if (chip == 1) {
		if (on)
			outl(PCDR, inl(PCDR) & ~0x01);
		else
			outl(PCDR, inl(PCDR) | 0x01);
	} else {
		if (on)
			outl(PCDR, inl(PFDR) & ~0x02);
		else
			outl(PCDR, inl(PFDR) | 0x02);
	}
}

static void cmd_hmm_status(int chip)
{
	char cmd[] = { 0xD7 };	// Read Status Register
	char reg;

	cmd_hmm_cs(chip, 1);
	
	spi_write(cmd, sizeof(cmd));
	spi_read(&reg, 1);

	cmd_hmm_cs(chip, 0);

	printf("Chip %d status: 0x%x\r\n", chip, reg);
}

static void cmd_hmm_readpage(int chip, int page)
{
	int i, x, c;
	int y = 0;

	char cmd[] = {	0xE8,				// Continuous read cmd
			0x00, 0x00, 0x00,		// Address
			0x00, 0x00, 0x00, 0x00 };	// Don't care

	char *buf;

	buf = malloc(528);
	if (buf == NULL) {
		printf("Unable to allocate buffer\r\n");
		goto out;
	}

	cmd[1] = (page & 0x1FC0) >> 6;
	cmd[2] = (page & 0x3F) << 2;
	cmd[3] = 0x00;	// first byte

	cmd_hmm_cs(chip, 1);

	spi_write(cmd, sizeof(cmd));
	spi_read(buf, 528);

	cmd_hmm_cs(chip, 0);

	for (i = 0, x = 0; i < 528; ++i) {
		if (x == 0)
			printf("%x| ", i);
		else if (x == 4)
			printf("   ");

		printf("%x ", buf[i]);

		if (++x >= 8) {
			x = 0;
			puts("");

			if (++y == 23) {
				stdio_buf_disable();
				printf("More...");
				c = getchar();
				stdio_buf_enable();
				puts("");

				y = 0;

				if (c == 'q')
					break;
			}
		}
	}

out:

	if (buf != NULL)
		free(buf);
}

static void cmd_hmm(int argc, char *argv[])
{
	int chip;

	if (argc < 3)
		goto out_err;

	chip = atoi(argv[1]);

	if (!strcmp(argv[2], "status")) {
		cmd_hmm_status(chip);
	} else if (!strcmp(argv[2], "readpage")) {
		if (argc != 4)
			goto out_err;

		cmd_hmm_readpage(chip, atoi(argv[3]));
	} else {
		goto out_err;
	}

	return;

out_err:

	printf("supported commands:\r\n");
	printf("\thmm <chip> readpage <page>\r\n");
	printf("\thmm <chip> status\r\n");
}
#endif // CONFIG_SPI

#ifdef CONFIG_USB
static inline void usb_print_string(struct usb_string_desc *d)
{
	int i = 0;

	for (i = 0; i < (d->bLength - 2) / 2; ++i)
		putchar(d->bString[(i * 2) + 1]);
}

static void cmd_usb_tree(void)
{
	struct usb_dev *dev;
	struct list *p;
	int i;

	for (p = usb_devices.next; p != NULL; p = p->next) {
		dev = (struct usb_dev *)p;

		printf("[%d] %s (%d)\r\n",
			dev->address,
			dev->device_desc.bDeviceClass > usb_class_str_max
				? "Unknown" : usb_class_str[dev->device_desc.bDeviceClass],
			dev->device_desc.bDeviceClass);
		printf("\tVID:PID %x:%x\r\n",
			dev->device_desc.idVendor,
			dev->device_desc.idProduct);

		printf("\t");
		if (dev->sProduct != NULL)
			usb_print_string(dev->sProduct);
		else
			printf("<unknown>");

		printf(" by ");

		if (dev->sManufacturer != NULL)
			usb_print_string(dev->sManufacturer);
		else
			printf("<unknown>");
		puts("");

		printf("\tS/N ");
		if (dev->sManufacturer != NULL)
			usb_print_string(dev->sSerialNumber);
		else
			printf("<unknown>");

		printf(" Rev %x.%x\r\n",
			(dev->device_desc.bcdDevice & 0xFF00) >> 8,
			dev->device_desc.bcdDevice & 0xFF);
			
		printf("\tConfigurations: %d\r\n",
			dev->device_desc.bNumConfigurations);

		for (i = 0; i < dev->device_desc.bNumConfigurations; ++i) {
			printf("\tC%d: %d mA ",
				dev->configuration_desc[i].bMaxPower * 2,
				dev->configuration_desc[i].bConfigurationValue);
			if (dev->configuration_desc[i].bmAttributes.remoteWakeup)
				printf("RemoteWakeup ");
			if (dev->configuration_desc[i].bmAttributes.selfPowered)
				printf("SelfPowered ");
			puts("");
		}
	}
}

static void cmd_usb(int argc, char *argv[])
{
	if (argc < 2)
		goto out_err;

	if (!strcmp(argv[1], "init")) {
		ohci_init();
	} else if (!strcmp(argv[1], "tree")) {
		cmd_usb_tree();
	} else {
		goto out_err;
	}

	return;

out_err:

	printf("supported commands:\r\n");
	printf("\tinit\r\n");
	printf("\ttree\r\n");
}
#endif // CONFIG_USB

struct command {
	const char *name;
	void (*func)(int, char **);
	const char *description;
};

static void cmd_help(int, char **);

struct command commands[] = {
	{ "?", cmd_help, "synonym for \"help\"" },
	{ "alarm", cmd_alarm, "test alarm: alarm <msec> <oneshot>" },
#ifdef CONFIG_NET
	{ "arp", cmd_arp, "display ARP cache" },
#endif
	{ "dumpmem",cmd_dumpmem, "dump memory location: dumpmem <addr> <len>" },
	{ "exit", cmd_exit, "exit the console" },
	{ "help", cmd_help, "list available commands" },
#ifdef CONFIG_SPI
	{ "hmm", cmd_hmm, "HMM commands" },
#endif
#ifdef CONFIG_NET
	{ "ifconfig", cmd_ifconfig, "configure Ethernet interfaces" },
#endif
	{ "kstat", cmd_kstat, "dump kernel statistics" },
#ifdef CONFIG_NAND
	{ "nand", cmd_nand, "NAND flash operations" },
#endif
#ifdef CONFIG_NET
	{ "netstat", cmd_netstat, "dump network statistics" },
#endif
	{ "ps", cmd_ps, "list running processes" },
	{ "reset", cmd_reset, "reset the system" },
#ifdef CONFIG_SPI
	{ "spi", cmd_spi, "SPI commands" },
#endif
#ifdef CONFIG_USB
	{ "usb", cmd_usb, "USB commands" },
#endif
	{ NULL, NULL }
};

static void cmd_help(int argc, char *argv[])
{
	struct command *cmd;
	int i;

	for (i = 0; commands[i].name != NULL; ++i) {
		cmd = &commands[i];

		printf("%s - %s\r\n", cmd->name, cmd->description);
	}
}

static void handle_command(int argc, char *argv[])
{
	struct command *cmd;
	int i;

	for (i = 0; commands[i].name != NULL; ++i) {
		cmd = &commands[i];

		// @@@ need strncmp
		if (!strcmp(cmd->name, argv[0])) {
			cmd->func(argc, argv);
			return;
		}
	}

	printf("unknown command: %s\r\n", argv[0]);
}

void console_task(void)
{
	char buf[128];
	int argc;
	char *argv[MAX_ARGS];
	char *p;
#ifdef _DBG
	int i;
#endif

	while (1) {
		stdio_buf_disable();

		if (!ticker_done)
			cmd_ticker(0, NULL);

		printf("> ");

		if (NULL == gets(buf, sizeof(buf))) {
			puts("error reading string");
		} else {
			puts("");

			// Ignore an empty string
			if (buf[0] == '\0')
				continue;

			stdio_buf_enable();

			// Break up the arguments
			argc = 0;
			memset(argv, 0, sizeof(char *) * MAX_ARGS);

			p = buf;
			while (*p != '\0') {
				// Find the first non-whitespace
				while (isspace(*p))
					*p++ = '\0';

				if (*p == '\0')
					break;

				argv[argc++] = p;

				// Find the next whitespace
				while (!isspace(*p) && *p != '\0')
					++p;
			}

			// We need at least one arg/cmd
			if (argc <= 0)
				continue;

#ifdef _DBG
			printf("argc is 0x%x\r\n", argc);
			for (i = 0; i < argc; ++i)
				printf("0x%x \"%s\"\r\n", i, argv[i]);
#endif

			// Command dispatch
			handle_command(argc, argv);
		}
	}
}

