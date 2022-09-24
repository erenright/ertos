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
 * dev/usb.c
 *
 * Universal Serial Bus Driver
 */

#include <usb.h>
#include <stdio.h>

#define LANG_EN_US	0x0409

uint8_t nextAddress = 1;

struct list usb_devices = {
	.next = NULL,
	.prev = NULL,
};

char *usb_pid_str[] = {
	"RESERVED",	// 0
	"OUT",		// 1
	"ACK",		// 2
	"DATA0",	// 3
	"PING",		// 4
	"SOF",		// 5
	"NYET",		// 6
	"DATA2",	// 7
	"SPLIT",	// 8
	"IN",		// 9
	"NAK",		// A / 10
	"DATA1",	// B / 11
	"PRE/ERR",	// C / 12
	"SETUP",	// D / 13
	"STALL",	// E / 14
	"MDATA",	// F / 15
};

char *usb_class_str[] = {
	"Unknown",	// 0
	"Unknown",	// 1
	"Unknown",	// 2
	"Unknown",	// 3
	"Unknown",	// 4
	"Unknown",	// 5
	"Unknown",	// 6
	"Unknown",	// 7
	"Unknown",	// 8
	"Hub",		// 9 / USB_CLASS_HUB
};
int usb_class_str_max = 9;	// Max index to usb_class_str

//******************************************************************************
//
// CRC5() - Computes a USB CRC5 value given an input value.
//          Ported from the Perl routine from the USB white paper entitled
//          "CYCLIC REDUNDANCY CHECKS IN USB"
//          www.usb.org/developers/whitepapers/crcdes.pdf
//
//          Ported by Ron Hemphill 01/20/06.
//
//  dwinput:    The input value.
//  iBitcnt:    The number of bits represented in dwInput.
//
//  Returns:    The computed CRC5 value.
//
//  Examples (from the white paper):
//      dwInput     iBitcnt     Returns:
//      0x547       11          0x17
//      0x2e5       11          0x1C
//      0x072       11          0x0E
//      0x400       11          0x17
//
//******************************************************************************
#define INT_SIZE    32      // Assumes 32-bit integer size
static unsigned usb_crc5(unsigned dwInput, int iBitcnt)
{
    const uint32_t poly5 = (0x05 << (INT_SIZE-5));
    uint32_t crc5  = (0x1f << (INT_SIZE-5));
    uint32_t udata = (dwInput << (INT_SIZE-iBitcnt));

    if ( (iBitcnt<1) || (iBitcnt>INT_SIZE) )    // Validate iBitcnt
        return 0xffffffff;

    while (iBitcnt--)
    {
        if ( (udata ^ crc5) & (0x1<<(INT_SIZE-1)) ) // bit4 != bit4?
        {
            crc5 <<= 1;
            crc5 ^= poly5;
        }
        else
            crc5 <<= 1;

        udata <<= 1;
    }

    // Shift back into position
    crc5 >>= (INT_SIZE-5);

    // Invert contents to generate crc field
    crc5 ^= 0x1f;

    return crc5;
}

static uint16_t usb_crc16(void *buffer, uint32_t count) 
{ 
	uint8_t *bp; 
	uint32_t poly, a, b, newBit, lostBit; int i; 
	poly = 0x8005; 
	a = 0xffff; 
	bp = buffer; 


	while(count--) 
	{ 
		b = *(bp++); 
		for(i = 0; i<8; i++) 
		{ 
			a <<= 1; 
			newBit = (b & 1); 
			b >>= 1; 
			lostBit = a >> 16; 

			if(newBit != lostBit) 
				a ^= poly; 

			a &= 0xffff; 
		} 
	} 

	return(a^0xffff); 
}

static inline int usb_token(struct usb_dev *dev, int pid, int ep)
{
	int crc5;
	struct usb_token_pkt *token = NULL;

	token = malloc(sizeof(struct usb_token_pkt));
	if (token == NULL) {
		printf("Unable to allocate SETUP token packet\r\n");
		return -1;
	}
	memset(token, 0, sizeof(struct usb_token_pkt));

	// Construct and queue the token packet
	crc5 = usb_crc5(dev->address << 4 | ep, 11);
	usb_fill_token_pkt(token, pid, dev->address, ep, crc5);
	if (dev->hcd->out(dev->address, 0, token, sizeof(struct usb_token_pkt))) {
		printf("Unable to queue token\r\n");
		free(token);
		return -1;
	}

	// HCD now owns token
	token = NULL;

	return 0;
}

static int usb_set_address(struct usb_dev *dev, int address)
{
	struct usb_request_pkt *req = NULL;

	// Issue SETUP request
	if (usb_token(dev, PID_SETUP, 0))
		return -1;

	// Construct and queue the DATA packet
	req = malloc(sizeof(struct usb_request_pkt));
	if (req == NULL) {
		printf("Unable to allocate token packet\r\n");
		return -1;
	}
	memset(req, 0, sizeof(struct usb_request_pkt));

	req->pid = PID_PACK(PID_DATA0);
	req->req.bmRequestType = 0;
	req->req.bRequest = USB_SET_ADDRESS;
	req->req.wValue = address;
	// req->req.wIndex = 0;
	// req->req.wLength = 0;
	req->crc16 = usb_crc16(&req->req, sizeof(struct usb_request_pkt) - 3);

	if (dev->hcd->out(dev->address, 0, req, sizeof(struct usb_request_pkt))) {
		printf("Unable to queue SET_ADDRESS SETUP data\r\n");
		goto out_err;
	}

	// Issue IN request
	if (usb_token(dev, PID_IN, 0))
		return -1;

	// Wait for completion at determine status
	// @@@ HACK for root hub!
	if (address > 1)
		wait(&dev->in_completion);

	// @@@ somehow determine status..?

	dev->address = address;

	return 0;

out_err:

	if (req != NULL)
		free(req);

	return -1;
}

static int usb_get_desc(struct usb_dev *dev,
	int type,
	void *desc,
	int len)
{
	struct usb_request_pkt *req = NULL;

	// Issue SETUP request
	if (usb_token(dev, PID_SETUP, 0))
		return -1;

	// Construct and queue the DATA packet
	req = malloc(sizeof(struct usb_request_pkt));
	if (req == NULL) {
		printf("Unable to allocate token packet\r\n");
		return -1;
	}
	memset(req, 0, sizeof(struct usb_request_pkt));

	req->pid = PID_PACK(PID_DATA0);
	req->req.bmRequestType = 0;
	req->req.direction = 1;		// IN
	req->req.bRequest = USB_GET_DESCRIPTOR;
	req->req.wValue = type << 8;
	// req->req.wIndex = 0;
	req->req.wLength = 18;
	req->crc16 = usb_crc16(&req->req, sizeof(struct usb_request_pkt) - 3);

	if (dev->hcd->out(dev->address, 0, req, sizeof(struct usb_request_pkt))) {
		printf("Unable to queue GET_DESCRIPTOR data\r\n");
		return -1;
	}

	// Issue IN request
	if (usb_token(dev, PID_IN, 0))
		return -1;

	if (dev->hcd->in(dev->address, 0, desc, len)) {
		printf("Unable to queue IN\r\n");
		return -1;
	}

	// Wait for completion and determine status
	// @@@ HACK for root hub!
	if (dev->address > 1)
		wait(&dev->in_completion);
	
	return 0;
}

static int usb_get_string_desc(struct usb_dev *dev,
	int idx,
	void *desc,
	int len)
{
	struct usb_request_pkt *req = NULL;

	// Issue SETUP request
	if (usb_token(dev, PID_SETUP, 0))
		return -1;

	// Construct and queue the DATA packet
	req = malloc(sizeof(struct usb_request_pkt));
	if (req == NULL) {
		printf("Unable to allocate token packet\r\n");
		return -1;
	}
	memset(req, 0, sizeof(struct usb_request_pkt));

	req->pid = PID_PACK(PID_DATA0);
	req->req.bmRequestType = 0;
	req->req.direction = 1;		// IN
	req->req.bRequest = USB_GET_DESCRIPTOR;
	req->req.wValue = (USB_DESC_STRING << 8) | idx;
	// req->req.wIndex = 0;
	req->req.wLength = 18;
	req->crc16 = usb_crc16(&req->req, sizeof(struct usb_request_pkt) - 3);

	if (dev->hcd->out(dev->address, 0, req, sizeof(struct usb_request_pkt))) {
		printf("Unable to queue GET_DESCRIPTOR data\r\n");
		return -1;
	}

	// Issue IN request
	if (usb_token(dev, PID_IN, 0))
		return -1;

	if (dev->hcd->in(dev->address, 0, desc, len)) {
		printf("Unable to queue IN\r\n");
		return -1;
	}

	// Wait for completion and determine status
	// @@@ HACK for root hub!
	if (dev->address > 1)
		wait(&dev->in_completion);
	
	return 0;
}

void usb_attach_device(struct usb_hcd *hcd)
{
	struct usb_dev *dev = NULL;
	int i;
	char buf[64];
	struct usb_string_desc *sd = (struct usb_string_desc *)buf;

	printf("New device attached on USB\r\n");

	// Create a new device
	dev = malloc(sizeof(struct usb_dev));
	if (dev == NULL) {
		printf("Unable to allocate usb_dev\r\n");
		goto out_err;
	}
	memset(dev, 0, sizeof(struct usb_dev));

	// @@@ need generic completion (and maybe dev?) init
	dev->in_completion.wait = bfifo_alloc(10);
	if (dev->in_completion.wait == NULL) {
		printf("Unable to allocation in_completion\r\n");
		goto out_err;
	}

	dev->hcd = hcd;
	dev->address = 0;
	
	dev->control_ep.bEndpointAddress = 0;
	dev->control_ep.wMaxPacketSize = 64;
	dev->control_ep.bmAttributes.transferType = USB_XFER_CONTROL;

	if (hcd->register_ep(&dev->control_ep)) {
		printf("Failed to register default control endpoint\r\n");
		goto out_err;
	}

	// Perform device enumeration (ref. USB 2.0 Chapter 9.1.2)
	if (usb_set_address(dev, nextAddress++)) {
		printf("Failed to assign address\r\n");
		goto out_err;
	} else {
		printf("New device assigned address %d\r\n", dev->address);
	}

#if 0
	// Remove the "default" control endpoint and assign the real one
	if (hcd->unregister_ep(&dev->control_ep)) {
		printf("Failed to remove the default control EP\r\n");
		goto out_err;
	}
#endif

	dev->control_ep.bEndpointAddress = dev->address;
	if (hcd->register_ep(&dev->control_ep)) {
		printf("Failed to register default control endpoint for device %d\r\n", dev->address);
		goto out_err;
	}

	// Acquire the device descriptor
	if (usb_get_desc(dev, USB_DESC_DEVICE, &dev->device_desc, sizeof(dev->device_desc))) {
		printf("Failed to read device descriptor\r\n");
		goto out_err;
	}

	if (dev->device_desc.iManufacturer > 0) {
		if (!usb_get_string_desc(dev, dev->device_desc.iManufacturer, buf, sizeof(buf))) {
			dev->sManufacturer = malloc(sd->bLength);
			if (dev->sManufacturer != NULL)
				memcpy(dev->sManufacturer, sd, sd->bLength);
		}
	}

	if (dev->device_desc.iProduct > 0) {
		if (!usb_get_string_desc(dev, dev->device_desc.iProduct, buf, sizeof(buf))) {
			dev->sProduct = malloc(sd->bLength);
			if (dev->sProduct != NULL)
				memcpy(dev->sProduct, sd, sd->bLength);
		}
	}

	if (dev->device_desc.iSerialNumber > 0) {
		if (!usb_get_string_desc(dev, dev->device_desc.iSerialNumber, buf, sizeof(buf))) {
			dev->sSerialNumber = malloc(sd->bLength);
			if (dev->sManufacturer != NULL)
				memcpy(dev->sSerialNumber, sd, sd->bLength);
		}
	}

	if (dev->device_desc.bNumConfigurations == 0) {
		printf("Device has no configurations\r\n");
		goto out_err;
	}

	// Retrieve all configuration descriptors
	dev->configuration_desc = malloc(sizeof(struct usb_configuration_desc)
						* dev->device_desc.bNumConfigurations);
	if (dev->configuration_desc == NULL) {
		printf("Failed to allocate configuration descriptors\r\n");
		goto out_err;
	}

	for (i = 0; i < dev->device_desc.bNumConfigurations; ++i) {
		if (usb_get_desc(dev,
				USB_DESC_CONFIGURATION,
				&dev->configuration_desc[i],
				sizeof(struct usb_configuration_desc))) {
			printf("Failed to get configuration descriptor %d\r\n", i);
			goto out_err;
		}
	}

	// Enumeration completed successfully, device is now registered and
	// operational
	list_add_after(&usb_devices, dev);

	return;

out_err:

	if (dev != NULL) {
		// @@@ need completion free
		if (dev->in_completion.wait != NULL)
			bfifo_free(dev->in_completion.wait);

		if (dev->configuration_desc != NULL)
			free(dev->configuration_desc);

		if (dev->sManufacturer != NULL)
			free(dev->sManufacturer);

		if (dev->sProduct != NULL)
			free(dev->sProduct);

		if (dev->sSerialNumber != NULL)
			free(dev->sSerialNumber);

		free(dev);
	}
}

