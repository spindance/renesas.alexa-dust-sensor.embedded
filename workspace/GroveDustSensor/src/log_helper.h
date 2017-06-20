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
* File Name    : log_helper.h
* Description  : This file simple debug logging utilities
*******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version Description
* : 25.05.2017 1.00 First Release
******************************************************************************/

#ifndef LOG_HELPER_H_
#define LOG_HELPER_H_

#include <stdbool.h>
#include <stdint.h>

void log_helper(bool to_lcd, bool is_error, const char *message, const char *file, const char *function, uint32_t line_num);

#define LOG(message) log_helper(false, false, message, __FILE__, __func__, __LINE__)
#define LOG_ERROR(message) log_helper(false, true, message, __FILE__, __func__, __LINE__)
#define LOG_LCD(message) log_helper(true, false, message, __FILE__, __func__, __LINE__)
#define LOG_LCD_ERROR(message) log_helper(true, true, message, __FILE__, __func__, __LINE__)

#endif /* LOG_HELPER_H_ */
