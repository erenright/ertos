/*
 * ohci.h
 */

#ifndef _OHCI_H
#define _OHCI_H

#include <types.h>

// Endpoint Descriptor (ref. OpenHCI Rev 1.0a Section 4.2.1)
struct ohci_ed {
	// Dword 0
	union {
		struct {
			uint32_t	FunctionAddress		: 7;
			uint32_t	EndpointNumber		: 4;
			uint32_t	Direction		: 2;
			uint32_t	Speed			: 1;
			uint32_t	Skip			: 1;
			uint32_t	Format			: 1;
			uint32_t	MaximumPacketSize	: 11;
			uint32_t	reserved0		: 5;
		};

		uint32_t dword0;
	};

	// Dword 1
	union {
		uint32_t	TDQueueTailPointer;
		uint32_t	dword1;
	};

	// Dword 2
	union {
		struct {
			uint32_t	Halted			: 1;
			uint32_t	toggleCarry		: 1;
			uint32_t	zero			: 2;
			uint32_t	unused2			: 28;
		};

		uint32_t		TDQueueHeadPointer;
		uint32_t		dword2;
	};

	// Dword 3
	union {
		uint32_t	NextED;
		uint32_t	dword3;
	};
} __attribute__((packed));

// Values for ohci_ed.Direction
#define DIRECTION_FROM_TD	0
#define DIRECTION_OUT		1
#define DIRECTION_IN		2

// Values for ohci_ed.Speed
#define SPEED_FS	0
#define SPEED_LS	1

// Values for ohci_ed.Format
#define ED_FMT_CTRL	0
#define ED_FMT_BULK	ED_FMT_CTRL
#define ED_FMT_INTR	ED_FMT_CTRL
#define ED_FMT_ISOC	1

// Transfer Descriptor (ref. OpenHCI rev 1.0a Section 4.3.1.1)
struct td {
	// Dword 0
	union {
		struct {
			uint32_t	reserved0	: 18;
			uint32_t	bufferRounding	: 1;
			uint32_t	PID		: 2; // Direction/PID
			uint32_t	DelayInterrupt	: 3;
			uint32_t	DataToggle	: 2;
			uint32_t	ErrorCount	: 2;
			uint32_t	ConditionCode	: 4;
		};

		uint32_t	dword0;
	};

	// Dword 1
	union {
		uint32_t	CurrentBufferPointer;
		uint32_t	dword1;
	};

	// Dword 2
	union {
		uint32_t	NextTD;
		uint32_t	dword2;
	};

	// Dword 3
	union {
		uint32_t	BufferEnd;
		uint32_t	dword3;
	};
} __attribute__((packed));

// Values for td.PID
#define OHCI_PID_SETUP	0
#define OHCI_PID_OUT	1
#define PHCI_PID_IN	2

// Values for td.ConditionCode
#define CC_NoError		0
#define CC_CRC			1
#define CC_BitStuffing		2
#define CC_DataToggleMismatch	3
#define CC_Stall		4
#define CC_DeviceNotResponding	5
#define CC_PIDCheckFailure	6
#define CC_UnexpectedPID	7
#define CC_DataOverrun		8
#define CC_DataUnderrun		9
// reserved			10
// reserved			11
#define CC_BufferOverrun	12
#define CC_BufferUnderrun	13
#define CC_NotAccessed		14

struct hcca {
	uint32_t	HccaInterruptTable[32];
	uint16_t	HccaFrameNumber;
	uint16_t	HccaPad1;
	uint32_t	HccaDoneHead;
	uint32_t	reserved[29];
} __attribute__((packed));

void ohci_init(void);

#endif // !_OHCI_H
