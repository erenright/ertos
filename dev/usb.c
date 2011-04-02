/*
 * dev/usb.c
 *
 * Universal Serial Bus Driver
 */

#include <usb.h>
#include <stdio.h>

uint8_t nextAddress = 1;

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

static int usb_get_device_desc(struct usb_dev *dev, struct usb_device_desc *desc)
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
	req->req.wValue = USB_DESC_DEVICE << 8;
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

	// Wait for completion at determine status
	// @@@ HACK for root hub!
	if (dev->address > 1)
		wait(&dev->in_completion);

	
	return 0;
}

void usb_attach_device(struct usb_hcd *hcd)
{
	struct usb_dev *dev = NULL;

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

	// Perform device enumeration (ref. @@@)
	if (usb_set_address(dev, nextAddress++)) {
		printf("Failed to assign address\r\n");
		goto out_err;
	} else {
		printf("Assigned address: %d\r\n", dev->address);
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
	if (usb_get_device_desc(dev, &dev->device_desc)) {
		printf("Failed to read device descriptor\r\n");
		goto out_err;
	}

	return;

out_err:

	if (dev != NULL) {
		// @@@ need completion free
		if (dev->in_completion.wait != NULL)
			bfifo_free(dev->in_completion.wait);
		free(dev);
	}
}

