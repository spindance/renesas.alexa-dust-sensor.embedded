/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products.
* No other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIESREGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED
* OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY
* LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE FOR ANY DIRECT,
* INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR
* ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability
* of this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2013, 2016 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/

/***********************************************************************************************************************
* File Name    : lcd_display_api.h
* Version      : 1.0
* Device(s)    : R7FS3A77C
* Tool-Chain   : GCC ARM Embedded v4.9.3.20150529
* OS           : ThreadX
* H/W Platform : Renesas Synergy S3A7 IoT Fast Prototyping Kit
* Description  : LCD Display API header file
* Creation Date: 12/15/2016
***********************************************************************************************************************/

#ifndef LCD_DISPLAY_API_H_
#define LCD_DISPLAY_API_H_

/***********************************************************************************************************************
Includes <System Includes> , “Project Includes”
***********************************************************************************************************************/
#include "sf_message_api.h"

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/
char fatalErrString[41];
#define MQTT_ERR_TRAP(a)     do {\
                                if(a) {\
                                    snprintf(fatalErrString, 41, "%.30s:%d:%u", __FILE__, __LINE__, a);\
                                    BufferLine(0, fatalErrString);\
                                    PaintText();\
                                    __asm("BKPT");\
                                }\
                            } while (0)

/***********************************************************************************************************************
Typedef Definitions
***********************************************************************************************************************/
typedef struct lcd_display_payload
{
    sf_message_header_t     header;     ///< Required header for messaging framework.
    unsigned int            line_number;///< line number to display text on.
    char                    msg[100];   ///< text to display.
} lcd_display_payload_t;

/***********************************************************************************************************************
Exported global variables
***********************************************************************************************************************/

/***********************************************************************************************************************
Exported global functions
***********************************************************************************************************************/
void ConfigureDisplayHardware565rgb(void);
void PaintScreen(uint8_t *p_data);
void PaintText();
int  BufferLine(int line, char *str);

#endif /* LCD_DISPLAY_API_H_ */
