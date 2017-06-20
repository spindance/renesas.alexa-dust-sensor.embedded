/*
 * ascii.h
 *
 *  Created on: Nov 7, 2016
 *      Author: gjacobso01
 */

#ifndef LCD_ASCII_H_
#define LCD_ASCII_H_
#include    <stdio.h>
/* -------------------------------------------------------------------------------- */
/* -- µGUI FONTS                                                                 -- */
/* -- Source: http://www.mikrocontroller.net/user/show/benedikt                  -- */
/* -------------------------------------------------------------------------------- */
typedef struct
{
   unsigned char* p;
   uint16_t char_width;
   uint16_t char_height;
} UG_FONT;
//#define  USE_FONT_4X6
//#define  USE_FONT_5X6
//#define  USE_FONT_5X12
//#define  USE_FONT_6X8
//#define  USE_FONT_6X10
//#define  USE_FONT_7X12
#define  USE_FONT_8X8
#define  USE_FONT_8X12
//#define  USE_FONT_8X14
#define  USE_FONT_10X16
//#define  USE_FONT_12X16
//#define  USE_FONT_16X26
//#define  USE_FONT_22X36
//#define  USE_FONT_24X40
//#define  USE_FONT_32X53

#ifdef USE_FONT_4X6
   extern const UG_FONT FONT_4X6;
#endif
#ifdef USE_FONT_5X8
   extern const UG_FONT FONT_5X8;
#endif
#ifdef USE_FONT_5X12
   extern const UG_FONT FONT_5X12;
#endif
#ifdef USE_FONT_6X8
   extern const UG_FONT FONT_6X8;
#endif
#ifdef USE_FONT_6X10
   extern const UG_FONT FONT_6X10;
#endif
#ifdef USE_FONT_7X12
   extern const UG_FONT FONT_7X12;
#endif
#ifdef USE_FONT_8X8
   extern const UG_FONT FONT_8X8;
#endif
#ifdef USE_FONT_8X12
   extern const UG_FONT FONT_8X12;
#endif
#ifdef USE_FONT_8X14
   extern const UG_FONT FONT_8X14;
#endif
#ifdef USE_FONT_10X16
   extern const UG_FONT FONT_10X16;
#endif
#ifdef USE_FONT_12X16
   extern const UG_FONT FONT_12X16;
#endif
#ifdef USE_FONT_12X20
   extern const UG_FONT FONT_12X20;
#endif
#ifdef USE_FONT_16X26
   extern const UG_FONT FONT_16X26;
#endif
#ifdef USE_FONT_22X36
   extern const UG_FONT FONT_22X36;
#endif
#ifdef USE_FONT_24X40
   extern const UG_FONT FONT_24X40;
#endif
#ifdef USE_FONT_32X53
   extern const UG_FONT FONT_32X53;
#endif


#endif /* LCD_ASCII_H_ */
