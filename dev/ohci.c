/*
 * dev/ochi.c
 *
 * OpenHCI Rev 1.0a Host Controller Driver
 */

#include "../arch/regs.h"

#include <stdio.h>
#include <types.h>
#include <sys/mem.h>
#include <sys/irq.h>
#include <string.h>

#include <ohci.h>
#include <usb.h>

static void *_hcca = NULL;
static struct hcca *hcca = NULL;

static struct usb_hub root_hub;

// For later pointer assignment
static uint8_t DeviceRemovable[2];
static uint8_t PortPwrCtrlMask[2];

#define RH_CONFIGURATION_DESC_LEN	9
#define RH_INTERFACE_DESC_LEN		9
#define RH_ENDPOINT_DESC_LEN		7
#define RH_HUB_DESC_LEN			(6+(2*2))

#define RH_CONFIGURATION_DESC_TOTAL_LEN \
		(RH_CONFIGURATION_DESC_LEN \
		+ RH_INTERFACE_DESC_LEN	\
		+ RH_ENDPOINT_DESC_LEN \
		+ RH_HUB_DESC_LEN)

// String descriptors
static const char *ohci_str[] = {
	"<null>",		// 0
	"EEnright",		// 1 / device.iManufacturer
	"OHCI Root Hub",	// 2 / device.iProduct
	"12345678",		// 3 / device.iSerialNumber
	"Hub",			// 4 / config.iConfiguration
};
static int ohci_str_max = 5;	// Maximum index into ohci_str

enum hub_state {
	HUB_IDLE = 0,
	HUB_SETUP,
	HUB_DATA,
	HUB_STATUS,
};

struct hub_data {
	int address;
	int new_address;
	char ep0_data[64];
	int ep0_len;
	enum hub_state state;
	int status;
};

static struct hub_data hub = {
	.address = 0,
	.new_address = 0,
	.state = HUB_IDLE,
	.status = 0,
	.ep0_len = 0,
};

// Returns zero if the root hubs were successfully probed
static int root_hub_probe(void)
{
	int rc = 0;
	uint32_t rhDescA = inl(HcRhDescriptorA);
	uint32_t rhDescB = inl(HcRhDescriptorB);

	memset(&root_hub, 0, sizeof(struct usb_hub));

	//
	// Generate the hub descriptor
	//

	// OHCI defines at most 15 ports, so just initialize enough bits
	// for 15 ports regardless of the real number
	root_hub.hub_desc.bLength = RH_HUB_DESC_LEN;
	root_hub.hub_desc.bDescriptorType = USB_DESC_HUB;
	root_hub.hub_desc.bNbrPorts = rhDescA & 0xFF;

	// OHCI supports a maximum of 15 ports
	if (root_hub.hub_desc.bNbrPorts > 15) {
		printf("Illegal number of ports in root hub: %d\r\n",
			root_hub.hub_desc.bNbrPorts);
		rc = -1;
		goto out;
	}

	root_hub.hub_desc.powerSwitchingMode = rhDescA & 0x10
		? USB_HUB_PSM_INDIVIDUAL
		: USB_HUB_PSM_GANGED;

	// OHCI forbids the root hub from being a compound device
	root_hub.hub_desc.compoundDevice = 0;

	root_hub.hub_desc.overCurrentProtectionMode = rhDescA & 0x80
		? 1
		: 0;

	// @@@ OHCI does not seem to define these fields?
	root_hub.hub_desc.ttThinkTime = USB_HUB_TTTT_8;
	// root_hub.hub_desc.portIndicatorsSupported = 0;

	root_hub.hub_desc.bPwrOn2PwrGood = (rhDescA & 0xFF000000) >> 24;

	// root_hub.hub_desc.bHubContrCurrent = 0;

	root_hub.hub_desc.DeviceRemovable = DeviceRemovable;
	DeviceRemovable[0] = (rhDescB & 0xFF000000) >> 24;
	DeviceRemovable[1] = (rhDescB & 0x0FF00000) >> 16;

	root_hub.hub_desc.PortPwrCtrlMask = PortPwrCtrlMask;
	// USB 2.0 states this should be all 1's (table 11-13)
	PortPwrCtrlMask[0] = 0xFF;
	PortPwrCtrlMask[1] = 0xFF;

	//
	// Generate the device descriptor
	//

	root_hub.device_desc.bLength = 18;
	root_hub.device_desc.bDescriptorType = USB_DESC_DEVICE;
	root_hub.device_desc.bcdUSB = 0x0200;
	root_hub.device_desc.bDeviceClass = USB_CLASS_HUB;
	// root_hub.device_desc.bDeviceSubClass = 0;
	// root_hub.device_desc.bDeviceProtocol = 0;
	root_hub.device_desc.bMaxPacketSize0 = 64;
	root_hub.device_desc.idVendor = 0xEEEE;
	root_hub.device_desc.idProduct = 0xAAAA;
	root_hub.device_desc.bcdDevice = 0x0001;
	root_hub.device_desc.iManufacturer = 1;
	root_hub.device_desc.iProduct = 2;
	root_hub.device_desc.iSerialNumber = 3;
	root_hub.device_desc.bNumConfigurations = 1;

	//
	// Generate the configuration descriptor
	//

	root_hub.configuration_desc.bLength = 9;
	root_hub.configuration_desc.bDescriptorType = USB_DESC_CONFIGURATION;
	root_hub.configuration_desc.wTotalLength = RH_CONFIGURATION_DESC_TOTAL_LEN;
	root_hub.configuration_desc.bNumInterfaces = 1;
	// root_hub.configuration_desc.bConfigurationValue = 0;
	 root_hub.configuration_desc.iConfiguration = 4;

	// root_hub.configuration_desc.bmAttributes.reserved0 = 0;
	// root_hub.configuration_desc.bmAttributes.remoteWakeup = 0;
	root_hub.configuration_desc.bmAttributes.selfPowered = 1;
	root_hub.configuration_desc.bmAttributes.reserved1 = 1;

	root_hub.configuration_desc.bMaxPower = 0;

	//
	// Generate the interface descriptor
	//

	root_hub.interface_desc.bLength = 9;
	root_hub.interface_desc.bDescriptorType = USB_DESC_INTERFACE;
	// root_hub.interface_desc.bInterfaceNumber = 0;
	// root_hub.interface_desc.bAlternateSetting = 0;
	root_hub.interface_desc.bNumEndpoints = 1;
	root_hub.interface_desc.bInterfaceClass = USB_CLASS_HUB;
	// root_hub.interface_desc.bInterfaceSubClass = 0;
	// root_hub.interface_desc.bInterfaceProtocol = 0;
	// root_hub.interface_desc.iInterface = 0;		// @@@

	//
	// Generate the status change endpoint descriptor
	//

	root_hub.status_change_endpoint_desc.bLength = 7;
	root_hub.status_change_endpoint_desc.bDescriptorType = USB_DESC_ENDPOINT;
	root_hub.status_change_endpoint_desc.bEndpointAddress = 1 | USB_EP_IN;
	// root_hub.status_change_endpoint_desc.bInterfaceNumber = 0;
	root_hub.status_change_endpoint_desc.bmAttributes.transferType = USB_XFER_INTR;
	root_hub.status_change_endpoint_desc.bmAttributes.transferType = USB_XFER_INTR;
	root_hub.status_change_endpoint_desc.wMaxPacketSize = 64;
	root_hub.status_change_endpoint_desc.bInterval = 0xFF;

out:
	return rc;
}

static void root_hub_status_change(uint32_t sreg, int port)
{
	int s = inl(sreg);
	int w = 0;

	if (s & HcRhPortStatus_CSC) {
		w |= HcRhPortStatus_CSC;
		printf("Device %sconnected on port %d\r\n",
			s & HcRhPortStatus_CCS ? "" : "dis",
			port);
	}

	// Clear source of interrupt, if any
	if (w)
		outl(sreg, w);
}

void ohci_isr(void)
{
	int s = inl(HcInterruptStatus);
	int w = 0;

	printf("ohci_isr: %x\r\n", s);

	if (s & HcInterruptEnable_RHSC) {
		// @@@ arch-specific
		root_hub_status_change(HcRhPortStatus1, 1);
		root_hub_status_change(HcRhPortStatus2, 2);

		w |= HcInterruptEnable_RHSC;
	}

	// Clear flagged interrupts
	outl(HcInterruptStatus, w);
}

static int hub_in(int ep, void *buf, off_t len)
{
	int rc = 0;

	switch (ep & 0x7F) {	// strip IN/OUT bit
	case 0:
		if (len > hub.ep0_len)
			len = hub.ep0_len;
		memcpy(buf, hub.ep0_data, len);
		hub.state = HUB_STATUS;
		hub.status = PID_ACK;
		break;

	case 1:
		// @@@
		break;

	default:
		rc = -1;
		break;
	}

	return rc;
}

static int ohci_in(int address, int ep, void *buf, off_t len)
{
	// Intercept messages for the root hub
	if (address == hub.address)
		return hub_in(ep, buf, len);
	else
		printf("device in needed\r\n");

	return 0;
}

static void create_string_desc(int idx)
{
	struct usb_string_desc *d = (struct usb_string_desc *)hub.ep0_data;
	int i;
	int len;

	if (idx > ohci_str_max) {
		printf("invalid index\r\n");
		hub.ep0_len = 0;
		return;
	}

	len = strlen(ohci_str[idx]);

	d->bLength = 2 + (len * 2);
	d->bDescriptorType = USB_DESC_STRING;

	for (i = 0; i < len; ++i) {
		d->bString[i * 2] = 0;
		d->bString[(i * 2) + 1] = ohci_str[idx][i];
	}

	hub.ep0_len = d->bLength;
}

static void get_descriptor(struct usb_request *req)
{
	int type = (req->wValue & 0xFF00) >> 8;
	void *p = NULL;

	switch (type) {
	case USB_DESC_DEVICE:
		hub.ep0_len = req->wLength > sizeof(root_hub.device_desc)
			? sizeof(root_hub.device_desc)
			: req->wLength;
		p = &root_hub.device_desc;
		break;

	case USB_DESC_CONFIGURATION:
		hub.ep0_len = req->wLength > sizeof(root_hub.configuration_desc)
			? sizeof(root_hub.configuration_desc)
			: req->wLength;
		p = &root_hub.configuration_desc;
		break;

	case USB_DESC_STRING:
		create_string_desc(req->wValue & 0xFF);
		break;

	default:
		printf("ohci: unhandled GET_DESCRIPTOR: %d\r\n", type);
		return;
	}

	if (p != NULL)
		memcpy(hub.ep0_data, p, hub.ep0_len);
	hub.state = HUB_DATA;
}

static void hub_handle_request(struct usb_request *req)
{
	switch (req->bRequest) {
	case USB_SET_ADDRESS:
		// Accept the new address (real assignment takes place after
		// transaction completion)
		hub.new_address = req->wValue;
		hub.status = PID_ACK;
		hub.state = HUB_STATUS;
		break;

	case USB_GET_DESCRIPTOR:
		get_descriptor(req);
		break;

	default:
		printf("hub_handle_request: unhandled request: %d\r\n", req->bRequest);
		break;
	};
}

static int hub_out(int ep, const void *buf, off_t len)
{
	int rc = 0;
	struct usb_token_pkt *pkt;
	struct usb_request_pkt *req;

	switch (ep & 0x7F) {	// strip IN/OUT bit
	case 0:
		pkt = (struct usb_token_pkt *)buf;
		//printf("hub_in: received %s token\r\n", pid_str[pkt->pid & 0x0F]);

		switch (pkt->pid & 0x0F) {
		case PID_SETUP:
			hub.state = HUB_SETUP;
			break;

		case PID_DATA0:
		case PID_DATA1:
			switch (hub.state) {
			case HUB_SETUP:
				// This data will be the request
				req = (struct usb_request_pkt *)buf;
				hub_handle_request(&req->req);
				break;

			default:
				break;
			};
			break;

		case PID_IN:
			switch (hub.state) {
			case HUB_STATUS:
				// Respond with status and go to idle
				// @@@ respond
				hub.state = HUB_IDLE;

				// New address must take effect only
				// after transaction completion
				if (hub.new_address) {
					hub.address = hub.new_address;
					hub.new_address = 0;
				}
				break;

			default:
				break;
			};
			break;

		};

		break;

	case 1:
		// @@@
		break;

	default:
		rc = -1;
		break;
	}

	return rc;
}

static int ohci_out(int address, int ep, const void *buf, off_t len)
{
	// Intercept messages for the root hub
	if (address == hub.address)
		return hub_out(ep, buf, len);
	else
		printf("device out needed\r\n");

	return 0;
}

int ohci_register_ep(struct usb_endpoint_desc *ep)
{
	// Intercept messages for the root hub
	if (ep->bEndpointAddress == hub.address)
		return 0;	// No need to register an EP for ourself

	return 0;
}

int ohci_unregister_ep(struct usb_endpoint_desc *ep)
{
	// Intercept messages for the root hub
	if (ep->bEndpointAddress == hub.address)
		return 0;	// No need to unregister an EP for ourself

	return 0;
}

static struct usb_hcd ohci_hcd = {
	.in = ohci_in,
	.out = ohci_out,
	.register_ep = ohci_register_ep,
	.register_ep = ohci_unregister_ep,
};

void ohci_init(void)
{
	int rev;
	int ctrl;
	int fmInterval;

	// @@@ arch-specific!
	// Enable clock to USB controller
	outl(PwrCnt, inl(PwrCnt) | PwrCnt_USH_EN);

	rev = inl(HcRevision);
	if ((rev & 0xFF) != 0x10) {
		printf("Unsupported OHCI revision: %d.%d\r\n",
			(rev >> 4) & 0xF,
			rev & 0xF);
		return;
	}

	// Determine that we own the controller
	ctrl = inl(HcControl);
	if (ctrl & HcControl_IR
		|| (ctrl & HcControl_HCFS_MASK) != HcControl_USBRESET) {

		printf("Something else owns the OHCI controller\r\n");
		return;
	}

	// Initialize HCCA
	_hcca = smalloc(512);	// 512 bytes guarantees we will get 256-byte alignment
	if (_hcca == NULL) {
		printf("Failed to allocate HCCA\r\n");
		return;
	}

	// Align HCCA to 256-byte boundary (ref. section 4.4)
	hcca = (struct hcca *)(_hcca + (256 - ((uint32_t)_hcca % 256)));
	memset(hcca, 0, sizeof(struct hcca));

	fmInterval = inl(HcFmInterval);

	// Reset HC @@@ this needs interrupts disabled as we have 10ms deadline!
	// @@@ ref Section 5.1.1.4
	// @@@ begin critical section
	outl(HcCommandStatus, inl(HcCommandStatus) | HcCommandStatus_HCR);
	while (inl(HcCommandStatus) & HcCommandStatus_HCR);

	outl(HcFmInterval, fmInterval);
	outl(HcHCCA, hcca);
	outl(HcInterruptEnable, 0xFFFFFFFF & ~HcInterruptEnable_SF);
	// HcControl
	// HcPeriodicStart
	outl(HcControl, inl(HcControl) | HcControl_USBOPERATIONAL);
	// @@@ end critical section

#if 0
	printf("HcRhStatus: %x\r\nHcRhDescriptorA: %x\r\nHcRhDescriptorB: %x\r\nHcRhPortStatus1: %x\r\nHcRhPortStatus2: %x\r\n",
		inl(HcRhStatus),
		inl(HcRhDescriptorA),
		inl(HcRhDescriptorB),
		inl(HcRhPortStatus1),
		inl(HcRhPortStatus2));
#endif

	// Register OHCI with USBD
	if (root_hub_probe())
		return;

	// Notify the USB subsystem that a new "device" has been attached.
	// It will probe it and determine that it is a hub.
	usb_attach_device(&ohci_hcd);

	// Good to go, enable interrupts
	/*outl(HcInterruptEnable,	HcInterruptEnable_RHSC
				| HcInterruptEnable_OC
				| HcInterruptEnable_MIE);*/
	register_irq_handler(USHINTR, ohci_isr, 0);
	enable_irq(USHINTR);

}
