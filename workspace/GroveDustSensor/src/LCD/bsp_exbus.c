/***********************************************************************************************************************
 * Copyright [2015] Renesas Electronics Corporation and/or its licensors. All Rights Reserved.
 *
 * The contents of this file (the �contents�) are proprietary and confidential to Renesas Electronics Corporation 
 * and/or its licensors (�Renesas�) and subject to statutory and contractual protections.
 *
 * Unless otherwise expressly agreed in writing between Renesas and you: 1) you may not use, copy, modify, distribute,
 * display, or perform the contents; 2) you may not use any name or mark of Renesas for advertising or publicity
 * purposes or in connection with your use of the contents; 3) RENESAS MAKES NO WARRANTY OR REPRESENTATIONS ABOUT THE
 * SUITABILITY OF THE CONTENTS FOR ANY PURPOSE; THE CONTENTS ARE PROVIDED �AS IS� WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT; AND 4) RENESAS SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, OR CONSEQUENTIAL DAMAGES,
 * INCLUDING DAMAGES RESULTING FROM LOSS OF USE, DATA, OR PROJECTS, WHETHER IN AN ACTION OF CONTRACT OR TORT, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE CONTENTS. Third-party contents included in this file may
 * be subject to different terms.
 **********************************************************************************************************************/
/**********************************************************************************************************************
* File Name    : bsp_exbus.c
* Description  : External Bus initialization.
***********************************************************************************************************************
* History : DD.MM.YYYY Version Description
*           03.23.2015 1.00    Initial Release.
***********************************************************************************************************************/
/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "bsp_api.h"

void bsp_exbus_init(void);
void R_BSP_WarmStart (bsp_warm_start_event_t event);

#ifdef EX_16BIT_BUS
void bsp_exbus_init(void)
{
	R_BUS->CSRC1[0].CSnCR_b.BSIZE = 0;		// 16 bit bus
	R_BUS->CSRC1[0].CSnCR_b.EMODE = 0;		// Little Endian
	R_BUS->CSRC0[0].CSnMOD_b.WRMOD = 1;		// Single Write Strobe
	R_BUS->CSRC0[0].CSnWCR2_b.CSWOFF = 0;	// Write-Access CS Extension Cycle Select=> 1wait
	R_BUS->CSRECEN = 0x3e00;				// No recovery cycle wait states
	R_BUS->CSRC0[0].CSnWCR1_b.CSPRWAIT = 0;
	R_BUS->CSRC0[0].CSnWCR1_b.CSPWWAIT = 0;
	R_BUS->CSRC0[0].CSnWCR1_b.CSRWAIT = 0;
	R_BUS->CSRC0[0].CSnWCR1_b.CSWWAIT = 0;

	R_BUS->CSRC1[0].CSnREC_b.WRCV =1;
	R_BUS->CSRECEN_b.RECVEN6 = 1;
	R_BUS->CSRECEN_b.RECVEN7 = 1;

}
#else
void bsp_exbus_init(void)
{
	R_BUS->CSRC1[0].CSnCR_b.BSIZE = 2;		// 8 bit bus
	R_BUS->CSRC1[0].CSnCR_b.EMODE = 1;		// Big Endian
	R_BUS->CSRC0[0].CSnMOD_b.WRMOD = 0;		// Byte Mode Strobe (0)
	R_BUS->CSRC0[0].CSnWCR2_b.CSWOFF = 0;	// Write-Access CS Extension Cycle Select=> 1wait
	R_BUS->CSRC0[0].CSnWCR2_b.WDOFF = 0;	// Write-Data Output Extension Cycle Select=> 1wait
	R_BUS->CSRECEN = 0x3e00;				// No recovery cycle wait states
	R_BUS->CSRC0[0].CSnWCR1_b.CSPRWAIT = 0;
	R_BUS->CSRC0[0].CSnWCR1_b.CSPWWAIT = 0;
	R_BUS->CSRC0[0].CSnWCR1_b.CSRWAIT = 0;
	R_BUS->CSRC0[0].CSnWCR1_b.CSWWAIT = 0;

	R_BUS->CSRC1[0].CSnREC_b.WRCV =1;		// Write Recovery cycle
	R_BUS->CSRECEN_b.RECVEN6 = 1;			// Write Recovery for same area write after write access
	R_BUS->CSRECEN_b.RECVEN7 = 1;			// Write Recovery for different area write after write access
}

#endif

/*
 *
 * 	Temporary measure until External bus support is in the BSP
 *
 */

void R_BSP_WarmStart (bsp_warm_start_event_t event)
{

if (event == BSP_WARM_START_PRE_C)
{
	bsp_exbus_init();
}
if (event == BSP_WARM_START_POST_C)
{
}
}
