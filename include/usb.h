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
 * include/usb.h
 *
 * Universal Serial Bus Driver
 */

#ifndef _USB_H
#define _USB_H

#include <types.h>
#include <sys/list.h>

#include <sleep.h>

//
// BEGIN DEFINITIONS FROM USB SPEC
//

// Device Descriptor (ref. USB 2.0 Chapter 9.6.1)
struct usb_device_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint16_t	bcdUSB;
	uint8_t		bDeviceClass;
	uint8_t		bDeviceSubClass;
	uint8_t		bDeviceProtocol;
	uint8_t		bMaxPacketSize0;
	uint16_t	idVendor;
	uint16_t	idProduct;
	uint16_t	bcdDevice;
	uint8_t		iManufacturer;
	uint8_t		iProduct;
	uint8_t		iSerialNumber;
	uint8_t		bNumConfigurations;
} __attribute__((packed));

// Device Qualifier Descriptor (ref. USB 2.0 Chapter 9.6.2)
struct usb_device_qualifier_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint16_t	bcdUSB;
	uint8_t		bDeviceClass;
	uint8_t		bDeviceSubClass;
	uint8_t		bDeviceProtocol;
	uint8_t		bMaxPacketSize0;
	uint8_t		bNumConfigurations;
	uint8_t		bReserved;
};

// Configuration Descriptor (ref. USB 2.0 Chapter 9.6.3)
struct usb_configuration_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint16_t	wTotalLength;
	uint8_t		bNumInterfaces;
	uint8_t		bConfigurationValue;
	uint8_t		iConfiguration;

	struct {
		uint8_t	reserved0	: 5;
		uint8_t	remoteWakeup	: 1;
		uint8_t	selfPowered	: 1;
		uint8_t	reserved1	: 1;
	} bmAttributes;

	uint8_t		bMaxPower;
} __attribute__((packed));

// Other Speed Configuration Descriptor (ref. USB 2.0 Chapter 9.6.4)
struct usb_other_speed_configuration_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint16_t	wTotalLength;
	uint8_t		bNumInterfaces;
	uint8_t		bConfigurationValue;
	uint8_t		iConfiguration;
	uint8_t		bmAttributes;
	uint8_t		bMaxPower;
};

// Interface Descriptor (ref. USB 2.0 Chapter 9.6.5)
struct usb_interface_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bInterfaceNumber;
	uint8_t		bAlternateSetting;
	uint8_t		bNumEndpoints;
	uint8_t		bInterfaceClass;
	uint8_t		bInterfaceSubClass;
	uint8_t		bInterfaceProtocol;
	uint8_t		iInterface;
};

// Endpoint Descriptor (ref. USB 2.0 Chapter 9.6.6)
struct usb_endpoint_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bEndpointAddress;

	struct {
		uint8_t	transferType	: 2;
		uint8_t	syncType	: 2;
		uint8_t	usageType	: 2;
		uint8_t	reserved0	: 2;
	} bmAttributes;

	uint16_t	wMaxPacketSize;
	uint8_t		bInterval;
};

// String Descriptor (ref. USB 2.0 Chapter 9.6.7)
struct usb_string_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;

	union {
		uint16_t	wLANGID[1];
		uint8_t		bString[2];
	};
} __attribute__((packed));

// Hub Descriptor (ref. USB 2.0 Chapter 11.23.2.1)
struct usb_hub_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bNbrPorts;

	union {
		struct {
			uint16_t	powerSwitchingMode		: 2;
			uint16_t	compoundDevice			: 1;
			uint16_t	overCurrentProtectionMode	: 2;
			uint16_t	ttThinkTime			: 2;
			uint16_t	portIndicatorsSupported		: 1;
			uint16_t	reserved			: 8;
		};

		uint16_t	wHubCharacteristics;
	};

	uint8_t		bPwrOn2PwrGood;
	uint8_t		bHubContrCurrent;
	uint8_t		*DeviceRemovable;	// malloc(bNbrPorts)
	uint8_t		*PortPwrCtrlMask;	// malloc(bNbrPorts)
};

// Hub port power switching mode
#define USB_HUB_PSM_GANGED	0
#define USB_HUB_PSM_INDIVIDUAL	1

// Hub over-current protection mode
#define USB_HUB_OCPM_GLOBAL	0
#define USB_HUB_OCPM_INDIVIDUAL	1

// Hub TT Think Time
#define USB_HUB_TTTT_8		0
#define USB_HUB_TTTT_16		1
#define USB_HUB_TTTT_24		2
#define USB_HUB_TTTT_32		3

// Descriptor types (ref. USB 2.0 Chapter 9.4, table 9-5)
#define USB_DESC_DEVICE				1
#define USB_DESC_CONFIGURATION			2
#define USB_DESC_STRING				3
#define USB_DESC_INTERFACE			4
#define USB_DESC_ENDPOINT			5
#define USB_DESC_DEVICE_QUALIFIER		6
#define USB_DESC_OTHER_SPEED_CONFIGURATION	7
#define USB_DESC_INTERFACE_POWER		8
#define USB_DESC_HUB				0x29	// Table 11-13

// Class types (ref ???)
#define USB_CLASS_HUB	9

#define USB_EP_IN	0x80
#define USB_EP_OUT	0x00

#define USB_XFER_CONTROL	0
#define USB_XFER_ISOC		1
#define USB_XFER_BULK		2
#define USB_XFER_INTR		3

// PIDs (ref. USB 2.0 Chapter 8.3.1, table 8-1)
#define PID_OUT		0x01
#define PID_IN		0x09
#define PID_SOF		0x05
#define PID_SETUP	0x0D

#define PID_DATA0	0x03
#define PID_DATA1	0x0B
#define PID_DATA2	0x07
#define PID_MDATA	0x0F

#define PID_ACK		0x02
#define PID_NAK		0x0A
#define PID_STALL	0x0E
#define PID_NYET	0x06

#define PID_PRE		0x0C
#define PID_ERR		PID_PRE
#define PID_SPLIT	0x08
#define PID_PING	0x04

#define PID_PACK(x)	(x | ((~x) << 4))

// Token packet (ref. USB 2.0 Chapter 8.4.1, figure 8.5)
// PID  : 8
// ADDR : 7
// ENDP : 4
// CRC5 : 5
struct usb_token_pkt {
	uint8_t		pid;
	uint16_t	data;
} __attribute__((packed));

static inline void usb_fill_token_pkt(struct usb_token_pkt *pkt,
	int pid, int addr, int endp, int crc5)
{
	pkt->pid = PID_PACK(pid);

	pkt->data = 0;
	pkt->data |= addr << 9;
	pkt->data |= endp << 5;
	pkt->data |= crc5;

	// @@@ swap bytes in pkt->data?
}

static inline void usb_parse_token_pkt(struct usb_token_pkt *pkt,
	int *pid, int *addr, int *endp, int *crc5)
{
	// @@@ swap bytes in pkt->data?

	*pid = pkt->pid & 0x0F;
	*addr = (pkt->data >> 9) & 0x7F;
	*endp = (pkt->data >> 5) & 0x0F;
	*crc5 = pkt->data & 0x1F;
}

// Standard Request Codes (rev. USB 2.0 Chapter 9.4, table 9-4)
#define USB_GET_STATUS		0
#define USB_CLEAR_FEATURE	1
// reserved			2
#define USB_SET_FEATURE		3
// reserved			4
#define USB_SET_ADDRESS		5
#define USB_GET_DESCRIPTOR	6
#define USB_SET_DESCRIPTOR	7
#define USB_GET_CONFIGURATION	8
#define USB_SET_CONFIGURATION	9
#define USB_GET_INTERFACE	10
#define USB_SET_INTERFACE	11
#define USB_SYNCH_FRAME		12

#define USB_DESC_DEVICE		1
#define	USB_DESC_CONFIGURATION	2
#define USB_DESC_STRING		3
#define USB_DESC_INTERFACE	4
#define USB_DESC_ENDPOINT	5

// Device Requests (ref. USB 2.0 Chapter 9.3, table 9-2)
struct usb_request {
	union {
		struct {
			uint8_t	recipient	: 5;
			uint8_t	type		: 2;
			uint8_t	direction	: 1;
		};

		uint8_t		bmRequestType;
	};

	uint8_t		bRequest;
	uint16_t	wValue;
	uint16_t	wIndex;
	uint16_t	wLength;
} __attribute__((packed));

struct usb_request_pkt {
	uint8_t pid;
	struct usb_request req;
	uint16_t crc16;
} __attribute__((packed));

// usb_request_pkt.direction
#define USB_REQUEST_HOST_TO_DEV	0
#define USB_REQUEST_DEV_TO_HOST	1

// usb_request_pkt.type
#define USB_REQUEST_STANDARD	0
#define USB_REQUEST_CLASS	1
#define USB_REQUEST_VENDOR	2

// usb_request_pkt.recipient
#define USB_REQUEST_DEVICE	0
#define USB_REQUEST_INTERFACE	1
#define USB_REQUEST_ENDPOINT	2
#define USB_REQUEST_OTHER	3

//
// END DEFINITIONS FROM USB SPEC
//

// USB Hub
struct usb_hub {
	struct list		list;

	struct usb_hub_desc		hub_desc;
	struct usb_device_desc		device_desc;
	struct usb_configuration_desc	configuration_desc;
	struct usb_interface_desc	interface_desc;
	struct usb_endpoint_desc	status_change_endpoint_desc;
};

// USB HCD driver (OHCI, etc)
struct usb_hcd {
	int (*out)(int address, int ep, const void *buf, off_t len);
	int (*in)(int address, int ep, void *buf, off_t len);
	int (*register_ep)(struct usb_endpoint_desc *ep);
	int (*unregister_ep)(struct usb_endpoint_desc *ep);
};

// A USB device
struct usb_dev {
	struct list	list;

	struct usb_hcd	*hcd;

	int		address;

	struct usb_endpoint_desc	control_ep;
	struct usb_device_desc		device_desc;
	struct usb_configuration_desc	*configuration_desc;

	// device_desc.iManufacturer
	struct usb_string_desc *sManufacturer;
	struct usb_string_desc *sProduct;
	struct usb_string_desc *sSerialNumber;

	struct completion in_completion;
};

extern struct list usb_devices;
extern char *usb_pid_str[];
extern char *usb_class_str[];
extern int usb_class_str_max;

void usb_attach_device(struct usb_hcd *hcd);

#endif // !_USB_H
