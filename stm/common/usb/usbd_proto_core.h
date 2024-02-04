#ifndef __USB_DFU_CORE_H_
#define __USB_DFU_CORE_H_

#include "usbd_ioreq.h"

enum
{
	DFU_DETACH = 0,
	DFU_DNLOAD = 1,
	DFU_UPLOAD,
	DFU_GETSTATUS,
	DFU_CLRSTATUS,
	DFU_GETSTATE,
	DFU_ABORT
};

void usb_init(void);
void usb_disconnect(void);
void usb_poll(uint32_t diff_ms);

#endif /* __USB_DFU_CORE_H_ */
