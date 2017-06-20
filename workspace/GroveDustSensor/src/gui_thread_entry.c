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
* File Name    : gui_thread_entry.c
* Version      : 1.0
* Device(s)    : R7FS3A77C
* Tool-Chain   : GCC ARM Embedded v4.9.3.20150529
* OS           : ThreadX
* H/W Platform : Renesas Synergy S3A7 IoT Fast Prototyping Kit
* Description  : This file implements GUI functions
* Creation Date: 12/15/2016
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes <System Includes> , “Project Includes”
***********************************************************************************************************************/
#include "gui_thread.h"
#include "lcd_display_api.h"
#include "app.h"

extern TX_THREAD sensor_thread;
/***********************************************************************************************************************
Exported global functions
***********************************************************************************************************************/
void gui_thread_entry(void);

/***********************************************************************************************************************
Private functions
***********************************************************************************************************************/
static void refresh_lcd_screen(void);

/***********************************************************************************************************************
Exported global variables
***********************************************************************************************************************/
extern const uint16_t m1logo[];
extern const uint16_t m1provision[];
extern const uint16_t m1provisionstart[];
extern int provisioning;
extern TX_THREAD sensor_thread;

/***********************************************************************************************************************
Private global variables
***********************************************************************************************************************/
static sf_message_header_t *p_message = NULL;

/***********************************************************************************************************************
* Function Name: refresh_lcd_screen
* Description  : Update LCD screen whenever user information change
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
static void refresh_lcd_screen(void)
{
    static const unsigned int SEC_PER_HOUR = 3600u;
    static const unsigned int SEC_PER_MINUTE = 60u;
    char lcd_string[25];
    unsigned int seconds = (unsigned int)g_elapsed_seconds;
    unsigned int h = seconds / SEC_PER_HOUR;
    unsigned int m = (seconds % SEC_PER_HOUR) / SEC_PER_MINUTE;
    unsigned int s = seconds - (h * SEC_PER_HOUR) - (m * SEC_PER_MINUTE);

    if (g_sensor_data.sensor_is_warmed_up)
    {
        /* Clear this line */
        BufferLine(SENSOR_INIT_STATUS_LCD_LINE_NUM, "\0");

        sprintf(lcd_string, "pcs/0.01 ft3: %u", (unsigned int)g_sensor_data.pcs);
        BufferLine(3, lcd_string);

        double ratio = (g_sensor_data.window_time_usec == 0u) ? 0.0 : (double)g_sensor_data.low_time_usec / ((double)g_sensor_data.window_time_usec);
        sprintf(lcd_string, "ratio:%.04f, rate:%us", ratio, (unsigned int)g_sensor_data.update_rate_sec);
        BufferLine(4, lcd_string);
    }

    sprintf(lcd_string, "%02u:%02u:%02u %u evt(s)", h, m, s, (unsigned int)g_sensor_data.window_count);
    BufferLine(5, lcd_string);

    PaintText();
}

/***********************************************************************************************************************
* Function Name: gui_thread_entry
* Description  : GUI thread entry function
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
void gui_thread_entry(void)
{
    ssp_err_t err;

    g_Blacklight_PWM.p_api->open(g_Blacklight_PWM.p_ctrl, g_Blacklight_PWM.p_cfg);
    g_Blacklight_PWM.p_api->dutyCycleSet(g_Blacklight_PWM.p_ctrl, 100, TIMER_PWM_UNIT_PERCENT, 0);

    ConfigureDisplayHardware565rgb();
    PaintScreen((uint8_t *) m1provision);
    err = g_sf_message0.p_api->pend(g_sf_message0.p_ctrl,
                                    &gui_thread_message_queue,
                                    (sf_message_header_t **) &p_message,
                                    200);
    if (err != SSP_SUCCESS)
    {
        PaintScreen((uint8_t *) m1logo);
        tx_thread_resume(&sensor_thread);
    }
    else
    {
        if ((SF_MESSAGE_EVENT_NEW_DATA == p_message->event_b.code) &&
            (p_message->event_b.class == SF_MESSAGE_EVENT_CLASS_TOUCH))
        {
            /* Drain any other messages */
            do {
                err = g_sf_message0.p_api->pend(g_sf_message0.p_ctrl,
                                                &gui_thread_message_queue,
                                                (sf_message_header_t **) &p_message,
                                                30);
                if (err == SSP_SUCCESS)
                {
                    g_sf_message0.p_api->bufferRelease(g_sf_message0.p_ctrl,
                                                       (sf_message_header_t *) p_message,
                                                       SF_MESSAGE_RELEASE_OPTION_NONE);
                }
            } while (err != SSP_ERR_MESSAGE_QUEUE_EMPTY);
            PaintScreen((uint8_t *) m1provisionstart);
            provisioning = 1;
        }
    }
    tx_semaphore_put(&g_provision_lock);

    while (1)
    {
        tx_thread_sleep(10);

        if (g_lcd_refresh_flag)
        {
            refresh_lcd_screen();
            g_lcd_refresh_flag = 0;
        }
    }
}
