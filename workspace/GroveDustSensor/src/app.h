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
* File Name    : app.h
* Version      : 1.0
* Description  : This is application header file
* Creation Date: 01/20/2017
***********************************************************************************************************************/

#ifndef _APP_H
#define _APP_H

/***********************************************************************************************************************
Includes <System Includes> , “Project Includes”
***********************************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

/******************************************************************************
Typedef definitions
******************************************************************************/
typedef struct
{
    /** Indicates whether an error occurred while processing the last window - "read only" */
    bool error;

    /** Particle concentration - "read only */
    uint32_t pcs;

    /** Low pulse time in the last window - "read only" */
    uint32_t low_time_usec;

    /** Duration of the last window - "read only" */
    uint32_t window_time_usec;

    /** A count of the number of windows (reporting intervals) - "read only" */
    uint32_t window_count;

    /** Indicates whether sensor warmup period has elapsed - "read only" */
    bool sensor_is_warmed_up;

    /** Low Pulse measurement window size. Initialized with a default value, but can
     * be updated. Changes to this will be reflected in the next measurement window. */
    uint32_t update_rate_sec;
} sensor_data_t;

/******************************************************************************
Macro definitions
******************************************************************************/
#define USEC_PER_SEC 1000000u

extern sensor_data_t g_sensor_data;
extern volatile int g_lcd_refresh_flag;
extern uint32_t g_elapsed_seconds;
extern const int SENSOR_INIT_STATUS_LCD_LINE_NUM;

#endif /* _APP_H */
