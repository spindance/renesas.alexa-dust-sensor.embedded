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

/***********************************************************************************************************************
 * File Name    : synergy_grapics_driver_565rgb.c
 * Description  : Graphics LCD driver - AS-IS integration with GUIX
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *           04.02.2015 1.00    Initial Release.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 **********************************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include "r_dmac.h"
#include "r_icu.h"
#include "r_transfer_api.h"
#include "r_external_irq_api.h"
#include "hal_data.h"
#include "gui_thread.h"
#include "ascii.h"
#include "lcd_display_api.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define DISPLAY_XRES             (240)
#define DISPLAY_YRES             (320)

#define FRAME_BUFFER_SIZE        (DISPLAY_XRES * DISPLAY_YRES * 2)
#define LCD_CMD  ( *(volatile uint8_t *) 0x80000000)
#ifdef EX_16BIT_BUS
#define LCD_DATA ( *(volatile uint16_t *) 0x80000004)
#else
#define LCD_DATA ( *(volatile uint8_t *) 0x80000004)
#endif
#define LCD_DATAW ( *(volatile uint16_t *) 0x80000004)

#define LCD_RESET    IOPORT_PORT_08_PIN_02
#define LCD_MODE8BIT IOPORT_PORT_08_PIN_03

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
typedef struct
{
    uint16_t Left;
    uint16_t Top;
    uint16_t Right;
    uint16_t Bottom;
}  DisplayRectangle_t;

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
void LCD_Display_On(void);
void LCD_Display_On(void);
void LCD_Display_Off(void);
void LCD_SetRectangle(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
void LCD_Fill_Rectangle(DisplayRectangle_t *Rect, uint16_t pixel_color);
void LCD_Memory_Write(uint16_t const *RGB565_Bitmap);
int display_lcd_string(const char *msg);

/***********************************************************************************************************************
 * Private global variables and functions
 **********************************************************************************************************************/
/** Frame buffers. each line of frame buffer have to be aligned to 64 bytes boundary. */

/** indicator for the number of visible frame buffer */

/** Synchronization object between drawing timing by GUIX and display timing by DMA driver */
static TX_SEMAPHORE          g_semaphore_dma_done;
///static TX_SEMAPHORE          g_semaphore_fmark;
///static bool g_guix_request_to_inform_available_timing_to_draw=false;

/** Graphics DMA callback function */
void dma_callback(transfer_callback_args_t * p_args);

///** Graphics Controller frame interrupt callback function */
///void fmark_callback(external_irq_callback_args_t * p_args);

/** Graphics LCD Controller Initialization **/
static void LCD_Init_ST7789S(void);

/** Graphics LCD, subroutines for register configuration */
#define BLOCK_SIZE 1024


void dma_callback(transfer_callback_args_t *p_args)
{
    SSP_PARAMETER_NOT_USED(p_args);
	tx_semaphore_ceiling_put(&g_semaphore_dma_done, 1);
}

#define TEXT_LINES 20
#define MAX_CHAR 8192

uint16_t g_render_buffer[768];    // Adjust for max font char size that we are going to render

static char textBuffer[TEXT_LINES][24 + 1] =
{
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0},
                                  {0}
};

uint8_t *gp_current_screen;

#define CHAR_PER_LINE ((240) / FONT_WIDTH)
#define MAX_LINES ((320) / FONT_HEIGHT)
#define COL_WHITE 0xffff
#define COL_BLACK 0x0000
#define SCREEN_STRIDE 240

//static UG_FONT * font = &FONT_8X12;
static const UG_FONT *font = &FONT_10X16;

static uint16_t g_curx = 0;
static uint16_t g_cury = 0;
#if 0
uint16_t g_font_colour = COL_WHITE;
uint16_t g_back_colour = COL_BLACK;
#else
uint16_t g_font_colour = COL_BLACK;
uint16_t g_back_colour = COL_WHITE;
#endif

static void display_write_pixel (uint16_t const x, uint16_t const y, uint16_t const colour)
{
    uint16_t x1;
    uint16_t y1;
    uint16_t *p_display = (uint16_t *)g_render_buffer;

    x1 = (uint16_t)(x + 0);
    y1 = (uint16_t)(y + 0);

    /* Write to the current buffer that is being displayed */
    if (((uint16_t)((y1 * font->char_width)) + x1) >= (sizeof(g_render_buffer) / 2))
    {
        MQTT_ERR_TRAP(-1);
    }
    p_display[((y1 * font->char_width)) + x1] = colour;
}

static void charput (uint8_t const val)
{
    uint16_t i, j, k, xo, yo, c, bn;
    uint8_t b, bt;
    unsigned char* p;

    bt = (uint8_t)val;

    switch (bt)
    {
       case 0xF6: bt = 0x94; break; // ö
       case 0xD6: bt = 0x99; break; // Ö
       case 0xFC: bt = 0x81; break; // ü
       case 0xDC: bt = 0x9A; break; // Ü
       case 0xE4: bt = 0x84; break; // ä
       case 0xC4: bt = 0x8E; break; // Ä
       case 0xB5: bt = 0xE6; break; // µ
       case 0xB0: bt = 0xF8; break; // °
    }
    if ((g_curx + font->char_width - 1) > DISPLAY_XRES)
    {
        g_cury = (uint16_t)(g_cury + font->char_height);
        g_curx = 0;
    }
    LCD_SetRectangle(g_curx, g_cury, (uint16_t)(g_curx + font->char_width - 1), (uint16_t)(g_cury + font->char_height - 1));
    yo = 0;
    bn = font->char_width;
    if (!bn)
    {
        return;
    }
    bn >>= 3;
    if (font->char_width % 8)
    {
        bn++;
    }
    p = font->p;
    p+= bt * font->char_height * bn;

    for (j = 0; j < font->char_height; j++)
    {
       xo = 0;
       c = font->char_width;
       for (i = 0; i < bn; i++)
       {
          b = *p++;
          for (k = 0; (k < 8) && c; k++)
          {
             if (b & 0x01)
             {
                 display_write_pixel(xo, yo, g_font_colour);
             }
             else
             {
                 display_write_pixel(xo, yo, g_back_colour);
             }
             b >>= 1;
             xo++;
             c--;
          }
       }
       yo++;
    }
    LCD_Memory_Write(g_render_buffer);
    g_curx = (uint16_t)(g_curx + font->char_width);
    if (g_curx > DISPLAY_XRES)
    {
        g_cury = (uint16_t)(g_cury + font->char_height);
    }
    if (g_cury > DISPLAY_YRES)
    {
        g_cury = 0;
    }
}

int display_lcd_string(const char *msg)
{
    uint8_t i, n;
    n = (uint8_t)strlen(msg);

    for (i = 0; i < n; i++)
        charput((uint8_t)msg[i]);

    return n;
}


void PaintScreen(uint8_t *p_data)
{
    ssp_err_t   err;

    LCD_SetRectangle(0, 0, (uint16_t)(DISPLAY_XRES - 1), (uint16_t)(DISPLAY_YRES - 1));
	LCD_CMD = 0x2C;		// Memory Write
	/* Sets flag to request LCD driver to inform available timing to start drawing
    g_guix_request_to_inform_available_timing_to_draw = true;
    tx_semaphore_get(&g_semaphore_fmark, TX_WAIT_FOREVER);
   */
    gp_current_screen = p_data;
	err = g_transfer_dma.p_api->reset(g_transfer_dma.p_ctrl, p_data, NULL, (320 * 240) / BLOCK_SIZE); // 320 x 240 x 2 / (1024 x 2)
    if(SSP_SUCCESS != err)
    {
        __asm("BKPT");
    }
    err = g_transfer_dma.p_api->start(g_transfer_dma.p_ctrl, TRANSFER_START_MODE_REPEAT);
    if(SSP_SUCCESS != err)
    {
        __asm("BKPT");
    }
    tx_semaphore_get(&g_semaphore_dma_done, TX_WAIT_FOREVER);
	LCD_CMD = 0x00;		// NOP to end the transfer
}

int BufferLine(int line, char *str)
{
    if (line > 12)
    {
        return -1;
    }

    strncpy(textBuffer[line], str, 24);
    textBuffer[line][24] = '\0';

    return 0;
}

void PaintText ()
{
    tx_mutex_get(&g_lcd_mutex, TX_WAIT_FOREVER);
    g_cury = 0;
    uint16_t max_len;
    uint16_t max_lines;

    max_len = (uint16_t)(DISPLAY_XRES/font->char_width);
//    max_lines = (uint16_t)(DISPLAY_YRES/font->char_height);
    max_lines = 12;

    for (int i = 0; i < max_lines - 1; i++)
    {
        g_curx = 0;
        int len = display_lcd_string(textBuffer[i]);
        // each line is 40 characters max
        // if we didn't print 40 characters, fill with spaces
        for (int j = 0; j < (max_len - len); j++)
            charput(' ');
        g_cury = (uint16_t)(g_cury + font->char_height);
    }
    tx_mutex_put(&g_lcd_mutex);
}

/***********************************************************************************************************************
 * @brief  GUIX display driver setup, initialize the graphics LCD panel hardware
 * @param[in]    p_display    Pointer to GUIX display interface structure
 * @retval  none
 **********************************************************************************************************************/
void ConfigureDisplayHardware565rgb (void)
{
    ssp_err_t   err;

	err = tx_semaphore_create(&g_semaphore_dma_done, NULL, 0);
	 if(SSP_SUCCESS != err)
	 {
	     __asm("BKPT");
	 }

    err = g_transfer_dma.p_api->open(g_transfer_dma.p_ctrl, g_transfer_dma.p_cfg);
    if(SSP_SUCCESS != err)
    {
        __asm("BKPT");
    }


    LCD_Init_ST7789S();

	LCD_Display_On();
 }


void LCD_Display_On(void)
{
	LCD_CMD = 0x29;		//
}

void LCD_Display_Off(void)
{
	LCD_CMD = 0x28;		//
}

static void LCD_Init_ST7789S(void)
{
    g_ioport.p_api->pinWrite(LCD_RESET, IOPORT_LEVEL_HIGH);
    g_ioport.p_api->pinWrite(LCD_RESET, IOPORT_LEVEL_LOW);
#ifdef EX_16BIT_BUS
    g_ioport.p_api->pinWrite(LCD_MODE8BIT, IOPORT_LEVEL_LOW);       // Set 16 bit I/F
#else
    g_ioport.p_api->pinWrite(LCD_MODE8BIT, IOPORT_LEVEL_HIGH);      // Set 8 bit I/F
#endif
    R_BSP_SoftwareDelay(100,BSP_DELAY_UNITS_MILLISECONDS);
    g_ioport.p_api->pinWrite(LCD_RESET, IOPORT_LEVEL_HIGH);
    R_BSP_SoftwareDelay(120,BSP_DELAY_UNITS_MILLISECONDS);

	LCD_CMD = 0x11;		// Exit Sleep
	R_BSP_SoftwareDelay(10,BSP_DELAY_UNITS_MILLISECONDS);

    LCD_CMD = 0x36;     // Memory Data Access Control
    LCD_DATA = 0x00;    // Row address order RGB color mode

    LCD_CMD = 0x3A;     // Interface Pixel Format
    LCD_DATA = 0x55;    // 16 bit per pixel, 65k RGB

    LCD_CMD = 0xB2;     // Porch Setting
    LCD_DATA = 0x0C;
    LCD_DATA = 0x0C;
    LCD_DATA = 0x00;
    LCD_DATA = 0x33;
    LCD_DATA = 0x33;

    LCD_CMD = 0xB7;     // Gate Control
    LCD_DATA = 0x35;

    LCD_CMD = 0xBB;     // VCOM Setting
    LCD_DATA = 0x2B;

    LCD_CMD = 0xC0;     // LCM Control
    LCD_DATA = 0x2C;

    LCD_CMD = 0xC2;     // VDV and VRH Command Enable
    LCD_DATA = 0x01;
    LCD_DATA = 0xFF;

    LCD_CMD = 0xC3;     // VRH Set
    LCD_DATA = 0x11;

    LCD_CMD = 0xC4;     // VDV Set
    LCD_DATA = 0x20;

    LCD_CMD = 0xC6;     // Frame Rate Control
    LCD_DATA = 0x0F;

    LCD_CMD = 0xD0;     // Power Control 1
    LCD_DATA = 0xA4;
    LCD_DATA = 0xA1;

    LCD_CMD = 0xE0;     // Positive Voltage Gamma control
    LCD_DATA = 0xD0;
    LCD_DATA = 0x00;
    LCD_DATA = 0x05;
    LCD_DATA = 0x0E;
    LCD_DATA = 0x15;
    LCD_DATA = 0x0D;
    LCD_DATA = 0x37;
    LCD_DATA = 0x43;
    LCD_DATA = 0x47;
    LCD_DATA = 0x09;
    LCD_DATA = 0x15;
    LCD_DATA = 0x12;
    LCD_DATA = 0x16;
    LCD_DATA = 0x19;

    LCD_CMD = 0xE1;     // Negative Voltage Gamma control
    LCD_DATA = 0xD0;
    LCD_DATA = 0x00;
    LCD_DATA = 0x05;
    LCD_DATA = 0x0D;
    LCD_DATA = 0x0C;
    LCD_DATA = 0x06;
    LCD_DATA = 0x2D;
    LCD_DATA = 0x44;
    LCD_DATA = 0x40;
    LCD_DATA = 0x0E;
    LCD_DATA = 0x1C;
    LCD_DATA = 0x18;
    LCD_DATA = 0x16;
    LCD_DATA = 0x19;

	LCD_CMD = 0x2A;		// X Address Set
	LCD_DATA = 0x00;	// 0-239
	LCD_DATA = 0x00;	//
	LCD_DATA = 0x00;	//
	LCD_DATA = 0xEF;	//

	LCD_CMD = 0x2B;		// Y Address Set
	LCD_DATA = 0x00;	// 0-319
	LCD_DATA = 0x00;	//
	LCD_DATA = 0x01;	//
	LCD_DATA = 0x3F;	//

	g_curx = 0;
	g_cury = 0;
}

void LCD_SetRectangle(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
    LCD_CMD = 0x2A;     // Column Address Set
    LCD_DATA = left >> 8;
    LCD_DATA = left & 0xFF;
    LCD_DATA = right >> 8;
    LCD_DATA = right & 0xFF;

    LCD_CMD = 0x2B;     // Row address set
    LCD_DATA = top >> 8;
    LCD_DATA = top & 0xFF;
    LCD_DATA = bottom >> 8;
    LCD_DATA = bottom & 0xFF;
}

void LCD_Fill_Rectangle(DisplayRectangle_t *Rect, uint16_t pixel_color)
{
    uint32_t size = (uint32_t)((Rect->Right - Rect->Left + 1) * (Rect->Bottom - Rect->Top + 1));
    uint32_t i;

    LCD_SetRectangle(Rect->Left, Rect->Top, Rect->Right, Rect->Bottom);

    LCD_CMD = 0x2C;     // Memory Write
    for (i = 0; i < size; i++)
    {
        LCD_DATA = pixel_color;
    }
    LCD_CMD = 0x00;     // NOP to end the transfer
}

void LCD_Memory_Write(uint16_t const *RGB565_Bitmap)
{
    uint32_t i;

    LCD_CMD = 0x2C;     // Memory Write
    for (i = 0; i < (uint32_t)(font->char_height * font->char_width); i++)
    {
        LCD_DATA = RGB565_Bitmap[i];
    }
    LCD_CMD = 0x00;     // NOP to end the transfer
}
