/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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

#include "startup.h"
#include <arm/omap2420.h>
#include <arm/omap3530.h>

/*
 * Initialize hwinfo structure in the system page.
 * This code is hardware dependant and may have to be changed
 * changed by end users.
 */
#define HDRC_ULPIREGDATA 		0x480ab074
#define HDRC_ULPIREGADDR 		0x480ab075
#define HDRC_ULPIREGCONTROL 	0x480ab076
#define HDRC_ULPIVBUSCONTROL 	0x480ab070
#define EHCI_ULPI_REG			0x480648a4
#define EHCI_CONFIG_REG			0x48064850
#define CONFIG_REG_CF			0x1

#define UHH_SYSCONFIG			0x48064010
#define UHH_HOSTCONFIG			0x48064040
#define OMAP_USBTLL_SYSCONFIG	0x48062010

#define LDELAY      	12000000
#define PLL_STOP        1	/* PER & IVA */
#define PLL_LOCK        7	/* MPU, IVA, CORE & PER */
#define OMAP35XX_CM_CLKEN2_PLL		0x48004D04
#define OMAP35XX_CM_IDLEST2_CKGEN	0x48004D24
#define OMAP35XX_CM_CLKSEL4_PLL		0x48004D4C
#define OMAP35XX_CM_CLKSEL5_PLL		0x48004D50
#define OMAP35XX_CM_FCLKEN_USBHOST 0x48005400
#define EN_USBHOST1			1<<0
#define EN_USBHOST2   		1<<1
#define OMAP35XX_CM_ICLKEN_USBHOST 0x48005410
#define EN_USBHOST   		0x1

#define RSTDELAY      	1000
#define OMAP35XX_GPIO5_OE		0x49056034
#define OMAP35XX_GPIO5_DATAOUT		0x4905603C
#define OMAP35XX_GPIO5		    128
#define USB332X_INTERFACECTL  0x7
#define USB332X_OTGCTL  0xa
#define USB332X_STP_PULLUP_DISABLE 0x90
#define USB332X_EXTERNALVBUS_ENABLE 0x86
#define TEB_USB_PHY_RESET_PIN		14

extern int teb;
extern int mtp;
extern int pin_mux;

uint8_t mentor_ulpi_read(uint8_t addr){
		int timeout = 10000000;
		uint8_t val=0;
	
		out8(HDRC_ULPIREGADDR, addr);
		out8(HDRC_ULPIREGCONTROL, 0x5);
		while(!(in8(HDRC_ULPIREGCONTROL) & 0x2) && timeout--)
			;
		if(timeout<=0){
			kprintf("ULPI read timeout at addr %x\n", addr);
		}else{
			val=in8(HDRC_ULPIREGDATA);
		}
	
		return val;
	}

int mentor_ulpi_write(uint8_t addr, uint8_t val){
	int timeout = 10000000;

	out8(HDRC_ULPIREGADDR, addr);
	out8(HDRC_ULPIREGDATA, val);
	out8(HDRC_ULPIREGCONTROL, 0x1);
	while(!(in8(HDRC_ULPIREGCONTROL) & 0x2) && timeout--)
		;
	if(timeout<=0){
		kprintf("ULPI write timeout at addr %x\n", addr);
		return -1;
	}
	return 0;
}
uint8_t EHCI_ViewPort_read(int port, uint8_t reg)
{

      uint8_t   val=0;
      int       i=100000;
      uint32_t regval=0x80000000|(reg<<16) | (port<<24)|(0x3<<22);
      out32(EHCI_ULPI_REG, regval); 
      while((in32(EHCI_ULPI_REG) & 0x80000000) && --i)
	  	;
      if(i<=0)
          kprintf(" EHCI_ViewPort_read timeout %x\n", in32(EHCI_ULPI_REG));
      val = in32(EHCI_ULPI_REG) & 0xFF;
      return val;

}

void EHCI_ViewPort_write(int port, uint8_t reg, uint8_t val)
{
      int i=100000;
      uint32_t regval=0x80000000|(reg<<16)|val|(port<<24)|(0x2<<22);
      out32(EHCI_ULPI_REG, regval); 
      while((in32(EHCI_ULPI_REG) & 0x80000000) && --i)
	  	;
      if(i<=0)
          kprintf(" EHCI_ViewPort_write timeout %x\n", in32(EHCI_ULPI_REG));
}

void
init_hwinfo()
{
	int count = LDELAY;
	int i;
	
	if(mtp){
		if(teb){ //teb board, we need to reset the EHCI PHY using GPIO14
			out32(OMAP3530_GPIO1_BASE+OMAP2420_GPIO_OE, 
				in32(OMAP3530_GPIO1_BASE+OMAP2420_GPIO_OE) & ~(1<<(TEB_USB_PHY_RESET_PIN)));
			out32(OMAP3530_GPIO1_BASE+OMAP2420_GPIO_DATAOUT, 
				in32(OMAP3530_GPIO1_BASE+OMAP2420_GPIO_DATAOUT) & ~(1<<(TEB_USB_PHY_RESET_PIN)));
			/* Hold the PHY in RESET for enough time till DIR is high */
			i= RSTDELAY;
			while(i--) in32(OMAP3530_GPIO1_BASE+OMAP2420_GPIO_DATAOUT);
		}
		/*Init HS USB -EHCI, OHCI clocks */
		out32(0x48005410,0x1); /*ICLK*/
		out32(0x48005400,0x3); /*FCLK*/

		/* Init OTG USB */
		out8(HDRC_ULPIVBUSCONTROL,0x3);
		mentor_ulpi_write(0x7,0x40);
		mentor_ulpi_write(0xa,0x86);

		/* perform TLL soft reset, and wait until reset is complete */
		/* (1<<3) = no idle mode only for initial debugging */
		out32(OMAP_USBTLL_SYSCONFIG,0x1a); /*OMAP_USBTLL_SYSCONFIG*/
		/* Wait for TLL reset to complete */
		while (!(in32(OMAP_USBTLL_SYSCONFIG+0x4) & 1));

		/* Put UHH in NoIdle/NoStandby mode */
		out32(UHH_SYSCONFIG,0x1108); /*UHH_SYSCONFIG */

		/* Bypass the TLL module for PHY mode operation */
		out32(UHH_HOSTCONFIG,0x21C); /*UHH_HOSTCONFIG -- bypass mode*/
		/* Ensure that BYPASS is set */
		while (in32(UHH_HOSTCONFIG) & 1);		

		if(teb){
			/* reset PHY: USB3320 */
			out32(OMAP3530_GPIO1_BASE+OMAP2420_GPIO_DATAOUT, 
				in32(OMAP3530_GPIO1_BASE+OMAP2420_GPIO_DATAOUT) |(1<<(TEB_USB_PHY_RESET_PIN)));
			/* Hold the PHY in RESET for enough time till PHY is settled and ready */
			i= RSTDELAY;
			while(i--) in32(OMAP3530_GPIO1_BASE+OMAP2420_GPIO_DATAOUT);
		}
		
		/*EHCI  Take the ownership */
		out32(EHCI_CONFIG_REG, 0x1);

		EHCI_ViewPort_write(2, 0x7,0x40);
		EHCI_ViewPort_write(2, 0xa,0x86);


		/*EHCI  release the ownership */
		out32(EHCI_CONFIG_REG, 0x0);
	}
	else if(pin_mux)
	{
		/*set GOIP_147 direction as output */
		out32(OMAP35XX_GPIO5_OE, in32(OMAP35XX_GPIO5_OE)& ~(1<<(147-OMAP35XX_GPIO5)));
		out32(OMAP35XX_GPIO5_DATAOUT, in32(OMAP35XX_GPIO5_DATAOUT)& ~(1<<(147-OMAP35XX_GPIO5)));
		/* Hold the PHY in RESET for enough time till DIR is high */
		i= RSTDELAY;
		while(i--) in32(OMAP35XX_GPIO5_DATAOUT);

		/*for beagle board revc*/
		/* set clock*/ 
		out32(OMAP35XX_CM_CLKEN_PLL, (in32(OMAP35XX_CM_CLKEN_PLL) & ~(0xf<<20))|(0x3<<20)); /* FREQSEL */
		out32(OMAP35XX_CM_CLKEN_PLL, (in32(OMAP35XX_CM_CLKEN_PLL) & ~(0x7<<16)) | (PLL_LOCK<<16));/* lock mode */
		count = LDELAY;
		while((!(in32(OMAP35XX_CM_IDLEST_CKGEN) & 2)) && count--);
	
		/* PER2 DPLL values 120M*/
		out32(OMAP35XX_CM_CLKEN2_PLL, (in32(OMAP35XX_CM_CLKEN2_PLL) & ~(0x7)) | (PLL_STOP));
		count = LDELAY;
		while((in32(OMAP35XX_CM_IDLEST2_CKGEN) & 1) && count--) 
		;
		out32(OMAP35XX_CM_CLKSEL4_PLL, (120<<8)|(12));/* set M2 */
		out32(OMAP35XX_CM_CLKSEL5_PLL, 1);/* set M2 */
		out32(OMAP35XX_CM_CLKEN2_PLL, (in32(OMAP35XX_CM_CLKEN2_PLL) & ~(0xf<<4))|(7<<4)); /* FREQSEL */
		out32(OMAP35XX_CM_CLKEN2_PLL, (in32(OMAP35XX_CM_CLKEN2_PLL) & ~(0x7)) | (PLL_LOCK));/* lock mode */
		count = LDELAY;
		while((!(in32(OMAP35XX_CM_IDLEST2_CKGEN) & 1)) && count--) ;
	
		/*Init HS USB -EHCI, OHCI clocks */
		out32(OMAP35XX_CM_ICLKEN_USBHOST, EN_USBHOST); /*ICLK*/
		out32(OMAP35XX_CM_FCLKEN_USBHOST,EN_USBHOST1 | EN_USBHOST2); /*FCLK*/

		/* perform TLL soft reset, and wait until reset is complete */
		/* (1<<3) = no idle mode only for initial debugging */
		out32(OMAP_USBTLL_SYSCONFIG,0x1a); /*OMAP_USBTLL_SYSCONFIG*/
		/* Wait for TLL reset to complete */
		while (!(in32(OMAP_USBTLL_SYSCONFIG+0x4) & 1));

		/* Put UHH in NoIdle/NoStandby mode */
		out32(UHH_SYSCONFIG,0x1108); /*UHH_SYSCONFIG */

		/* Bypass the TLL module for PHY mode operation */
		out32(UHH_HOSTCONFIG,0x21C); /*UHH_HOSTCONFIG -- bypass mode*/
		/* Ensure that BYPASS is set */
		while (in32(UHH_HOSTCONFIG) & 1);		

		/* reset PHY: USB3326 */
		out32(OMAP35XX_GPIO5_DATAOUT, in32(OMAP35XX_GPIO5_DATAOUT) |(1<<(147-OMAP35XX_GPIO5)));
		/* Hold the PHY in RESET for enough time till PHY is settled and ready */
		i= RSTDELAY;
		while(i--) in32(OMAP35XX_GPIO5_DATAOUT);

		/*EHCI  Take the ownership */
		out32(EHCI_CONFIG_REG, CONFIG_REG_CF);
				
		EHCI_ViewPort_write(2, USB332X_INTERFACECTL,USB332X_STP_PULLUP_DISABLE); /*configured to disable the integrated STP pull-up resistor */
		EHCI_ViewPort_write(2, USB332X_OTGCTL,USB332X_EXTERNALVBUS_ENABLE); /*enable external VBUS*/

		/*EHCI  release the ownership */
		out32(EHCI_CONFIG_REG, ~CONFIG_REG_CF);
	
	}
	set_syspage_section(&lsp.cpu.arm_boxinfo, sizeof(*lsp.cpu.arm_boxinfo.p));
}

__SRCVERSION( "$URL: http://svn/product/tags/public/bsp/nto641/ti-omap3530-beagle/1.0.0/src/hardware/startup/boards/omap3530/init_hwinfo.c $ $Rev: 249398 $" );
