/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
 *  
 * Licensed under the Apache License, Version 2.0 (the "License"). You  
 * may not reproduce, modify or distribute this software except in  
 * compliance with the License. You may obtain a copy of the License  
 * at: http://www.apache.org/licenses/LICENSE-2.0  
 *  
 * Unless required by applicable law or agreed to in writing, software  
 * distributed under the License is distributed on an "AS IS" basis,  
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied. 
 * 
 * This file may contain contributions from others, either as  
 * contributors under the License or as licensors under other terms.   
 * Please review this entire file for other proprietary rights or license  
 * notices, as well as the QNX Development Suite License Guide at  
 * http://licensing.qnx.com/license-guide/ for other information. 
 * $ 
 */








#include "externs.h"
#include <sys/mman.h>

void
set_port(unsigned port, unsigned mask, unsigned data) 
{
	unsigned char c;

	c = read_omap(port);
	write_omap(port, (c & ~mask) | (data & mask));
}

static void
clear_device(const uintptr_t *port) 
{
	write_omap(port[OMAP_UART_IER], 0);					// Disable all interrupts
	read_omap(port[OMAP_UART_LSR]);						// Clear Line Status Interrupt
	read_omap(port[OMAP_UART_RHR]);						// Clear RX Interrupt
	read_omap(port[OMAP_UART_THR]);						// Clear TX Interrupt
	read_omap(port[OMAP_UART_MSR]);						// Clear Modem Interrupt
}

//
// Clean up the device then add it to the interrupt list and enable it.
//
void
ser_attach_intr(DEV_OMAP *dev) {
	uintptr_t	*port = dev->port;

	// According to the National bug sheet you must wait for the transmit
	// holding register to be empty.
	do {
	} while((read_omap(port[OMAP_UART_LSR]) & OMAP_LSR_TXRDY) == 0);

	clear_device(port);

	dev->iid = InterruptAttach(dev->intr, ser_intr, dev, 0, 0);

	// Enable interrupt sources except transmit interrupt.
	write_omap(port[OMAP_UART_IER], OMAP_IER_RHR|OMAP_IER_LS|OMAP_IER_MS);
}


DEV_OMAP *
create_device(TTYINIT_OMAP *dip, unsigned unit, unsigned maxim_xcvr_kick) {
	DEV_OMAP 			*dev;
	unsigned			i;
	uintptr_t			port;
	unsigned char		msr;

	// Get a device entry and the input/output buffers for it.
	if ((dev = malloc(sizeof(*dev))) == NULL)
	{
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "io-char: Allocation of device entry failed (%d)", errno);
		return (dev);
	}	
	memset(dev, 0, sizeof(*dev));

	// Get buffers.
	dev->tty.ibuf.head = dev->tty.ibuf.tail = dev->tty.ibuf.buff = malloc(dev->tty.ibuf.size = dip->tty.isize);
	if (dev->tty.ibuf.buff == NULL)
	{
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "io-char: Allocation of input buffer failed (%d)", errno);
		free(dev);
		return (NULL);
	}
						   
	dev->tty.obuf.head = dev->tty.obuf.tail = dev->tty.obuf.buff = malloc(dev->tty.obuf.size = dip->tty.osize);
	if (dev->tty.obuf.buff == NULL)
	{
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "io-char: Allocation of output buffer failed (%d)", errno);
		free(dev->tty.ibuf.buff);
		free(dev);
		return (NULL);
	}

	dev->tty.cbuf.head = dev->tty.cbuf.tail = dev->tty.cbuf.buff = malloc(dev->tty.cbuf.size = dip->tty.csize);
	if (dev->tty.cbuf.buff == NULL)
	{
		slogf(_SLOG_SETCODE(_SLOGC_CHAR, 0), _SLOG_ERROR, "io-char: Allocation of canonical buffer failed (%d)", errno);
		free(dev->tty.ibuf.buff);
		free(dev->tty.obuf.buff);
		free(dev);
		return (NULL);
	}

	if (dip->tty.highwater)
		dev->tty.highwater = dip->tty.highwater;
	else
		dev->tty.highwater = dev->tty.ibuf.size - FIFO_SIZE * 2;

	strcpy(dev->tty.name, dip->tty.name);

	dev->tty.baud = dip->tty.baud;
	dev->tty.fifo = dip->tty.fifo;
	dev->tty.verbose = dip->tty.verbose;

	port = mmap_device_io(OMAP_UART_SIZE, dip->tty.port);
	for (i = 0; i < OMAP_UART_SIZE; i += 4)
		dev->port[i] = port + i;

	dev->intr = dip->tty.intr;
	dev->clk = dip->tty.clk;
	dev->div = dip->tty.div;

	dev->tty.flags = EDIT_INSERT | LOSES_TX_INTR;
	dev->tty.c_cflag = dip->tty.c_cflag;
	dev->tty.c_iflag = dip->tty.c_iflag;
	dev->tty.c_lflag = dip->tty.c_lflag;
	dev->tty.c_oflag = dip->tty.c_oflag;

	// Initialize termios cc codes to an ANSI terminal.
	ttc(TTC_INIT_CC, &dev->tty, 0);

	// Initialize the device's name.
	// Assume that the basename is set in device name.  This will attach
	// to the path assigned by the unit number/minor number combination
	unit = SET_NAME_NUMBER(unit) | NUMBER_DEV_FROM_USER;
	ttc(TTC_INIT_TTYNAME, &dev->tty, unit);

	// Initialize power management structures before attaching ISR
	ttc(TTC_INIT_POWER, &dev->tty, 0);

	// see if we have a maxim rs-232 transceiver that needs to be
	// kicked after it goes to sleep
	dev->kick_maxim = maxim_xcvr_kick;

	// Only setup IRQ handler for non-pcmcia devices.
	// Pcmcia devices will have this done later when card is inserted.
	if (dip->tty.port != 0 && dip->tty.intr != _NTO_INTR_SPARE) {
		// enable the UART
		write_omap(dev->port[OMAP_UART_MDR1], OMAP_MDR1_MODE_16X);
		// enable and clear fifo's
		write_omap(dev->port[OMAP_UART_LCR], OMAP_LCR_DLAB);
		write_omap(dev->port[OMAP_UART_DLL], 0);
		write_omap(dev->port[OMAP_UART_DLH], 0);
		write_omap(dev->port[OMAP_UART_FCR], OMAP_FCR_ENABLE|OMAP_FCR_RXCLR|OMAP_FCR_TXCLR);
		ser_stty(dev);
		ser_attach_intr(dev);
	}

	/* Set modem control lines to a ready state */
    	set_port(dev->port[OMAP_UART_MCR], OMAP_MCR_DTR|OMAP_MCR_RTS, OMAP_MCR_DTR|OMAP_MCR_RTS);

	if(dip->loopback){   
		set_port(dev->port[OMAP_UART_MCR], OMAP_MCR_LOOPBACK, OMAP_MCR_LOOPBACK);
	}

	// get current MSR stat
	msr = read_omap(dev->port[OMAP_UART_MSR]);

	// Initialize power management
	seromap_power_init(dev, dip);

	if(msr & OMAP_MSR_DDCD)
		tti(&dev->tty, (msr & OMAP_MSR_DCD) ? TTI_CARRIER : TTI_HANGUP);
					
	if((msr & OMAP_MSR_DCTS)  &&  (dev->tty.c_cflag & OHFLOW))
		tti(&dev->tty, (msr & OMAP_MSR_CTS) ? TTI_OHW_CONT : TTI_OHW_STOP);

	// Attach the resource manager
	ttc(TTC_INIT_ATTACH, &dev->tty, 0);

	return (dev);
}

__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/devc/seromap/init.c $ $Rev: 249398 $" );
