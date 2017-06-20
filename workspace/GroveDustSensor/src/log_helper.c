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
* File Name    : log_helper.c
* Description  : This file implements simple debug logging utilities
*******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version Description
* : 25.05.2017 1.00 First Release
******************************************************************************/

/******************************************************************************
Includes <System Includes> , “Project Includes”
******************************************************************************/
#include "log_helper.h"
#include "lcd_display_api.h"
#include <stdio.h>
#include <string.h>

/******************************************************************************
Private global variables and functions
******************************************************************************/
static const uint32_t line_width = 24;
static const int start_line = 5;
static const char * error_str = "ERROR: ";

static void console_log(bool is_error, const char *message, const char *file, const char *function, uint32_t line_num);

/******************************************************************************
* Function Name: log_helper
* Description : Logging utility that displays a message, to the console and
*   optionally to the LCD, and file/function/line number information
* Arguments : to_lcd:
*               - Determines whether message is printed to LCD
*             is_error:
*               - Determines whether to Indicates that an error occurred
*             message:
*               - A message to print
*             file, function, line_num: The file, function name and line_num
*             to display in the message
* Return Value : none
******************************************************************************/
void log_helper(bool to_lcd, bool is_error, const char *message, const char *file, const char *function, uint32_t line_num)
{
    if (to_lcd)
    {
        /* file is not logged to the LCD in order to save space */
        char buffer[line_width];
        int line = start_line;
        memset(buffer, 0, sizeof(buffer));

        if (is_error)
        {
            BufferLine(line++, (char *)error_str);
        }
        if (0 != strlen(message))
        {
            BufferLine(line++, (char *)message);
        }
        BufferLine(line++, (char *)function);
        sprintf(buffer, "line %u", (unsigned int)line_num);
        BufferLine(line++, buffer);
        PaintText();
    }

    console_log(is_error, message, file, function, line_num);
}

/******************************************************************************
* Function Name: console_log
* Description : Logging utility that displays to the console
* Arguments : See above
* Return Value : none
******************************************************************************/
static void console_log(bool is_error, const char *message, const char *file, const char *function, uint32_t line_num)
{
    /* TODO: printf not implemented? */
    printf("%s%s: %s, %s, line %u", is_error ? error_str : "", message, file, function, (unsigned int)line_num);
}
