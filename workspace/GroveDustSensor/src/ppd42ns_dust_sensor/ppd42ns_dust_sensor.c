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
* File Name    : ppd42ns_dust_sensor_support.c
* Description  : This file implements support for the Grove PPD42NS Dust Sensor
*******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version Description
* : 25.05.2017 1.00 First Release
******************************************************************************/

/******************************************************************************
Includes <System Includes> , “Project Includes”
******************************************************************************/
#include "ppd42ns_dust_sensor.h"
#include <math.h>

/******************************************************************************
Private global variables and functions
******************************************************************************/
static const ioport_level_t SENSOR_INACTIVE_LEVEL = IOPORT_LEVEL_HIGH;
static const ioport_level_t SENSOR_ACTIVE_LEVEL = IOPORT_LEVEL_LOW;

/******************************************************************************
* Function Name: calculate_ppd24ns_dust_concentration
* Description : Calculates dust concentration in pcs/0.01 cubic feet. Refer to
*   the Grove Dust Sensor user manual. Assumes the two values passed in are in
*   the same time units.
* Arguments : low_pulse_duration:
*               - Time during the specified window when the sensor output low
*             window_duration:
*               - Time window during which low pulse was measured
* Return Value : pcs/0.01 ft3
******************************************************************************/
uint32_t calculate_ppd24ns_dust_concentration(uint32_t low_pulse_duration, uint32_t window_duration)
{
    double percent = ((double)low_pulse_duration / (double)window_duration) * 100.0;
    double concentration = (1.1 * pow(percent, 3)) - (3.8 * pow(percent, 2)) + (520 * percent) + 0.62;
    return (uint32_t)concentration;
}

/******************************************************************************
* Function Name: is_ppd24ns_sensor_active
* Description : Returns whether dust sensor is "active", i.e. outputting low
* Arguments : pin:
*               - The GPIO RX pin the desired sensor is attached to
* Return Value : See above
******************************************************************************/
bool is_ppd24ns_sensor_active(ioport_port_pin_t pin)
{
    ioport_level_t new_level = SENSOR_INACTIVE_LEVEL;
    g_ioport.p_api->pinRead(pin, &new_level);
    return new_level == SENSOR_ACTIVE_LEVEL;
}
