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
* File Name    : sensor_thread_entry.c
* Version      : 1.0
* Device(s)    : R7FS3A77C
* Tool-Chain   : GCC ARM Embedded v4.9.3.20150529
* OS           : ThreadX
* H/W Platform : Renesas Synergy S3A7 IoT Fast Prototyping Kit
* Description  : This file implements application use sensirion sensor functions
* Creation Date: 03/01/2017
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes <System Includes> , “Project Includes”
***********************************************************************************************************************/
#include <led_support.h>
#include "ppd42ns_dust_sensor.h"
#include "app.h"
#include "nx_api.h"
#include "lcd_display_api.h"
#include "sensor_thread.h"

/******************************************************************************
Typedef definitions
******************************************************************************/
typedef struct s_lpo_window_data
{
    bool busy;
    uint32_t window_start_sec;
    uint32_t window_start_usec;
    uint32_t low_pulse_start_usec;
    uint32_t low_pulse_duration_usec;
} lpo_window_data_t;

/******************************************************************************
Macro definitions
******************************************************************************/
/** Enables skipping the warmup period */
#define WAIT_FOR_SENSOR_WARMUP true
#define DEFAULT_LPO_WINDOW_SIZE_SEC 15u

/***********************************************************************************************************************
Exported global function and variables
***********************************************************************************************************************/
extern void publish_notification(sensor_data_t user);
void sensor_thread_entry(void);
void sensor_output_change_callback(external_irq_callback_args_t * p_args);

/******************************************************************************
* Description : Global sensor data
******************************************************************************/
sensor_data_t g_sensor_data =
{
    .error = false,
    .pcs = 0u,
    .low_time_usec = 0u,
    .window_time_usec = 0u,
    .window_count = 0u,
    .sensor_is_warmed_up = WAIT_FOR_SENSOR_WARMUP ? false : true,
    .update_rate_sec = DEFAULT_LPO_WINDOW_SIZE_SEC
};

volatile int g_lcd_refresh_flag = 0;
const int SENSOR_INIT_STATUS_LCD_LINE_NUM = 3;
uint32_t g_elapsed_seconds = 0u;

/******************************************************************************
* Description : Private sensor low pulse occupancy data for a given time window
******************************************************************************/
static lpo_window_data_t lpo_window_data =
{
    .busy = true, // Initialize to true to indicate "not ready"
    .window_start_sec = 0u,
    .window_start_usec = 0u,
    .low_pulse_start_usec = 0u,
    .low_pulse_duration_usec = 0u
};

/******************************************************************************
Other Private variables and functions
******************************************************************************/
static const led_t UNHEALTHY_LED = LED0_RED;
static const led_t HEALTHY_LED = LED1_YELLOW;
static const led_t WINDOW_PERIOD_LED = LED2_GREEN;
static const led_t LOW_PULSE_LED = LED3_BLUE;
static const uint32_t SENSOR_WARMUP_TIME_SEC = 60u; // From sensor datasheet
static const timer_size_t TOGGLE_TIMER_PERIOD = 1u;

/* PPD42NS is expected on Grove A, whose RX pin supports interrupts */
static const ioport_port_pin_t SENSOR_PIN = IOPORT_PORT_04_PIN_08;

static void reset_time(void);
static uint32_t elapsed_time_usec(void);
static void report_dust_concentration(uint32_t low_pulse_usec, uint32_t window_usec);
static void write_text_to_line(char *text, int line_number);

/***********************************************************************************************************************
* Function Name: sensor_thread_entry
* Description  : Entry function of sensor Thread.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
void sensor_thread_entry(void)
{
    write_text_to_line("GROVE DUST SENSOR", 2);

    // Turn LEDs off and determine the sensor output pin
    (void)init_leds();
    turn_all_leds_on(false);

    /* Initialize and start the toggle timer */
    ssp_err_t result = g_toggle_timer.p_api->open(g_toggle_timer.p_ctrl, g_toggle_timer.p_cfg);
    result |= g_toggle_timer.p_api->periodSet(g_toggle_timer.p_ctrl, TOGGLE_TIMER_PERIOD, TIMER_UNIT_PERIOD_SEC);
    result |= g_toggle_timer.p_api->start(g_toggle_timer.p_ctrl);

    /* Initialize and start the elapsed time timer */
    result |= g_elapsed_time_timer.p_api->open(g_elapsed_time_timer.p_ctrl, g_elapsed_time_timer.p_cfg);
    result |= g_elapsed_time_timer.p_api->periodSet(g_elapsed_time_timer.p_ctrl, UINT32_MAX, TIMER_UNIT_PERIOD_USEC);
    result |= g_elapsed_time_timer.p_api->start(g_elapsed_time_timer.p_ctrl);

    /* Initialize the sensor pin interrupt */
    result |= g_sensor_input_irq.p_api->open(g_sensor_input_irq.p_ctrl, g_sensor_input_irq.p_cfg);

    bool initialized = SSP_SUCCESS == result;

    if (!initialized)
    {
        /* Close drivers to prevent unwanted callbacks */
        (void)g_toggle_timer.p_api->close(g_toggle_timer.p_ctrl);
        (void)g_elapsed_time_timer.p_api->close(g_elapsed_time_timer.p_ctrl);
        (void)g_sensor_input_irq.p_api->close(g_sensor_input_irq.p_ctrl);
    }

    write_text_to_line(initialized ? "SENSOR WARMING UP..." : "INITIALIZATION FAILED!", SENSOR_INIT_STATUS_LCD_LINE_NUM);
    turn_led_on(initialized ? HEALTHY_LED : UNHEALTHY_LED, true);
}

/******************************************************************************
* Function Name: toggle_timer_callback
* Description : Callback registered for g_toggle_timer
* Arguments : none
* Return Value : none
******************************************************************************/
void toggle_timer_callback(timer_callback_args_t * p_args)
{
    (void)p_args;
    g_elapsed_seconds++;

    if (!g_sensor_data.sensor_is_warmed_up)
    {
       /* Determine if the warmup period has elapsed. */
       if (g_elapsed_seconds >= SENSOR_WARMUP_TIME_SEC)
       {
           reset_time();
           g_sensor_data.sensor_is_warmed_up = true;
           lpo_window_data.busy = false; // Was set to true in init
       }

       /* Toggle the low pulse LED until sensor is warmed up */
       turn_led_on(LOW_PULSE_LED, g_elapsed_seconds % 2);
    }
    else
    {
        uint32_t start = lpo_window_data.window_start_usec;
        uint32_t end = elapsed_time_usec();
        uint32_t start_seconds = lpo_window_data.window_start_sec;

        /* Do a sanity check on the time values */
        if ((end > start) && (g_elapsed_seconds > start_seconds))
        {
            if ((g_elapsed_seconds - start_seconds) >= g_sensor_data.update_rate_sec)
            {
                lpo_window_data.busy = true;
                report_dust_concentration(lpo_window_data.low_pulse_duration_usec, end - start);

                /* Reset variables */
                reset_time();
                lpo_window_data.busy = false;
            }
        }

        /* Toggle the window timer LED after the sensor is warmed up */
        turn_led_on(WINDOW_PERIOD_LED, g_elapsed_seconds % 2);
    }

    g_lcd_refresh_flag = 1;
}

/******************************************************************************
* Function Name: sensor_output_change_callback
* Description : Callback registered for a low->high or high->low change for the
*   sensor low pulse output pin.
* Arguments : none
* Return Value : none
******************************************************************************/
void sensor_output_change_callback(external_irq_callback_args_t * p_args)
{
    if (g_sensor_data.sensor_is_warmed_up && !lpo_window_data.busy)
    {
        (void)p_args;
        bool is_active = is_ppd24ns_sensor_active(SENSOR_PIN);
        uint32_t now = elapsed_time_usec();

        if (is_active)
        {
            /* Mark the start of a low pulse */
            lpo_window_data.low_pulse_start_usec = now;
        }
        else if (now > lpo_window_data.low_pulse_start_usec)
        {
            /* Low pulse ended, add in its duration, reset the elapsed time timer */
            uint32_t low_time = now - lpo_window_data.low_pulse_start_usec;
            lpo_window_data.low_pulse_duration_usec += low_time;
        }
        /* if the above else if fails, the timer has overflowed/rolled over, so we'll simply ignore this low pulse. */

        turn_led_on(LOW_PULSE_LED, is_active);
    }
}

/******************************************************************************
* Function Name: elapsed_time_usec
* Description : Returns "relative" time in microseconds, based upon g_elapsed_time_timer.
*   NOTE: This timer is not accurate in terms real time (usec). E.g., 15s of
*   actual time equates to about 21s with this timer, hence the qualifier "relative".
* Arguments : none
* Return Value : See above
******************************************************************************/
static uint32_t elapsed_time_usec(void)
{
    timer_size_t time = 0u;
    timer_size_t counter = 0u;

    if (SSP_SUCCESS == g_elapsed_time_timer.p_api->counterGet(g_elapsed_time_timer.p_ctrl, &counter))
    {
        time = counter;
    }

    return time;
}

/******************************************************************************
* Function Name: report_dust_concentration
* Description : Reports dust concentration by updating the globabl dust sensor
*   data, and publishing the update to the GUI and Net threads.
* Arguments : low_pulse_usec:
*               - Low pulse time for the given time window
*             window_usec:
*               - Time window
* Return Value : none
******************************************************************************/
static void report_dust_concentration(uint32_t low_pulse_usec, uint32_t window_usec)
{
    g_sensor_data.error = low_pulse_usec > window_usec;
    g_sensor_data.pcs = calculate_ppd24ns_dust_concentration(low_pulse_usec, window_usec);
    g_sensor_data.low_time_usec = low_pulse_usec;
    g_sensor_data.window_time_usec = window_usec;
    g_sensor_data.window_count++;

    /* Publish via Net thread */
    publish_notification(g_sensor_data);
}

/******************************************************************************
* Function Name: write_text_to_line
* Description : Write the specified text to the specified line on the LCD.
* Arguments : See above
* Return Value : none
******************************************************************************/
static void write_text_to_line(char *text, int line_number)
{
    BufferLine(line_number, text);
    PaintText();
}

/******************************************************************************
* Function Name: reset_time
* Description : Resets the elapsed time timer, marks the start of a new low
*   pulse occupancy window, and clears out the accumulated low pulse time.
* Arguments : none
* Return Value : none
******************************************************************************/
static void reset_time(void)
{
    g_elapsed_time_timer.p_api->reset(g_elapsed_time_timer.p_ctrl);
    lpo_window_data.window_start_sec = g_elapsed_seconds;
    lpo_window_data.window_start_usec = elapsed_time_usec();
    lpo_window_data.low_pulse_duration_usec = 0u;
}
