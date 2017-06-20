/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2017 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/

/*******************************************************************************
* File Name    : led_support.c
* Description  : This file implements simple LED helper functions
*******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version Description
* : 25.05.2017 1.00 First Release
******************************************************************************/

/******************************************************************************
Includes <System Includes> , “Project Includes”
******************************************************************************/
#include "led_support.h"
#include "hal_data.h"

/******************************************************************************
Private global variables and functions
******************************************************************************/
static void set_led_level(led_t led, ioport_level_t level);

static bsp_leds_t leds;

/******************************************************************************
* Function Name: init_leds
* Description : Initializes data structures used by this module
* Arguments : none
* Return Value : Returns whether leds are configured for this board
******************************************************************************/
bool init_leds(void)
{
    /* Get LED information for this board */
    R_BSP_LedsGet(&leds);
    return 0 != leds.led_count;
}

/******************************************************************************
* Function Name: turn_led_on
* Description : Turns specified LED on or off
* Arguments : led:
*               - The LED to turn on/off
*             turn_on:
*               - Indicates whether LED is turned on or off
* Return Value : none
******************************************************************************/
void turn_led_on(led_t led, bool turn_on)
{
    set_led_level(led, turn_on ? IOPORT_LEVEL_HIGH : IOPORT_LEVEL_LOW);
}

/******************************************************************************
* Function Name: turn_all_leds_on
* Description : Turns all LEDs on or off
* Arguments : turn_on:
*               - Indicated whether to turn all LEDs on or off
* Return Value : none
******************************************************************************/
void turn_all_leds_on(bool turn_on)
{
    for(uint32_t i = 0; i < leds.led_count; i++)
    {
        turn_led_on(i, turn_on);
    }
}

/******************************************************************************
* Function Name: set_led_level
* Description : Sets the specified LED to the specified level
* Arguments : led:
*               - LED to modify
*             level:
*               - Level to set the specified LED to
* Return Value : none
******************************************************************************/
static void set_led_level(led_t led, ioport_level_t level)
{
    if ((led < leds.led_count) && (led < LED_MAX))
    {
        g_ioport.p_api->pinWrite(leds.p_leds[led], level);
    }
}
