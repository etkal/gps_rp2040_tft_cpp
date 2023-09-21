/*
 * Pico ILI934x TFT display driver class
 *
 * (c) 2023 Erik Tkal
 *
 * Modified from Darren Horrocks version to fix command/data/select timing, as well
 * as removing the GFXFont support.
 * Modified to inherit from Framebuffer (itself a version modified from analyzing
 * the Damien P. George MicroPython modframebuf.c).
 *
 * Notes:
 *   1) In order to use framebuf, always set width=240, height=320, rot=90/270.
 *      Let the framebuf return its width and height for calculation purposes.
 */

/*
BSD 3-Clause License

Copyright (c) 2022, Darren Horrocks
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <pico/stdlib.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include "framebuf.h"

#define _RDDSDR    0x0f // Read Display Self-Diagnostic Result
#define _SLPOUT    0x11 // Sleep Out
#define _GAMSET    0x26 // Gamma Set
#define _DISPOFF   0x28 // Display Off
#define _DISPON    0x29 // Display On
#define _CASET     0x2a // Column Address Set
#define _PASET     0x2b // Page Address Set
#define _RAMWR     0x2c // Memory Write
#define _RAMRD     0x2e // Memory Read
#define _MADCTL    0x36 // Memory Access Control
#define _VSCRSADD  0x37 // Vertical Scrolling Start Address
#define _PIXSET    0x3a // Pixel Format Set
#define _PWCTRLA   0xcb // Power Control A
#define _PWCRTLB   0xcf // Power Control B
#define _DTCTRLA   0xe8 // Driver Timing Control A
#define _DTCTRLB   0xea // Driver Timing Control B
#define _PWRONCTRL 0xed // Power on Sequence Control
#define _PRCTRL    0xf7 // Pump Ratio Control
#define _PWCTRL1   0xc0 // Power Control 1
#define _PWCTRL2   0xc1 // Power Control 2
#define _VMCTRL1   0xc5 // VCOM Control 1
#define _VMCTRL2   0xc7 // VCOM Control 2
#define _FRMCTR1   0xb1 // Frame Rate Control 1
#define _DISCTRL   0xb6 // Display Function Control
#define _ENA3G     0xf2 // Enable 3G
#define _PGAMCTRL  0xe0 // Positive Gamma Control
#define _NGAMCTRL  0xe1 // Negative Gamma Control

#define MADCTL_MY  0x80 ///< Bottom to top
#define MADCTL_MX  0x40 ///< Right to left
#define MADCTL_MV  0x20 ///< Reverse Mode
#define MADCTL_ML  0x10 ///< LCD refresh Bottom to top
// #define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
// #define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH       0x04 ///< LCD refresh right to left

#define MADCTL_RGB      0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR      0x08 ///< Blue-Green-Red pixel order

#define _MAX_CHUNK_SIZE 4096

/* Windows 16 colour pallet converted to 5-6-5 */
#define COLOUR_BLACK      0x0000
#define COLOUR_MAROON     0x8000
#define COLOUR_GREEN      0x0400
#define COLOUR_OLIVE      0x8400
#define COLOUR_NAVY       0x0010
#define COLOUR_PURPLE     0x8010
#define COLOUR_TEAL       0x0410
#define COLOUR_SILVER     0xC618
#define COLOUR_GRAY       0x8410
#define COLOUR_RED        0xF800
#define COLOUR_LIME       0x07E0
#define COLOUR_YELLOW     0xFFE0
#define COLOUR_BLUE       0x001F
#define COLOUR_FUCHSIA    0xF81F
#define COLOUR_AQUA       0x07FF
#define COLOUR_WHITE      0xFFFF

#define COLOUR_ORDER_RGB  MADCTL_RGB
#define COLOUR_ORDER_BGR  MADCTL_BGR

#define ILI934X_HW_WIDTH  240
#define ILI934X_HW_HEIGHT 320

enum ILI934X_ROTATION
{
    R0DEG,
    R90DEG,
    R180DEG,
    R270DEG,
    MIRRORED0DEG,
    MIRRORED90DEG,
    MIRRORED180DEG,
    MIRRORED270DEG
};

class ILI934X : public Framebuf
{
public:
    ILI934X(spi_inst_t* spi,
            uint8_t cs,
            uint8_t dc,
            uint8_t rst,
            uint16_t width            = 240,
            uint16_t height           = 320,
            ILI934X_ROTATION rotation = R0DEG,
            uint8_t colour_order      = COLOUR_ORDER_BGR);

    void reset();
    void init();
    void setRotation(ILI934X_ROTATION rotation = R0DEG);

    //    void setPixel(uint16_t x, uint16_t y, uint16_t colour);
    //    void fillRect(uint16_t x, uint16_t y, uint16_t h, uint16_t w, uint16_t colour);
    //    void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
    //    void drawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

    void clear(uint16_t colour = COLOUR_BLACK);
    //    void blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *bltBuf);
    //    void show(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void show();
    void show(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

    // uint16_t width() {return _width;}
    // uint16_t height() {return _height;}

    static inline uint16_t colour565(uint8_t r, uint8_t g, uint8_t b)
    {
        return (((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | ((b >> 3) & 0x1f);
    }

private:
    void _write(uint8_t cmd, uint8_t* data = NULL, size_t dataLen = 0);
    void _data(uint8_t* data, size_t dataLen = 0);
    void _writeBlock(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t* data = NULL, size_t dataLen = 0);
    inline void _cs_select()
    {
        if (_cs < 255)
        {
            sleep_us(1);
            gpio_put(_cs, 0); // Active low
            sleep_us(15);
        }
    }
    inline void _cs_deselect()
    {
        if (_cs < 255)
        {
            sleep_us(1);
            gpio_put(_cs, 1);
            sleep_us(1);
        }
    }
    inline void _command_select()
    {
        gpio_put(_dc, 0);
    }
    inline void _data_select()
    {
        gpio_put(_dc, 1);
    }

private:
    spi_inst_t* _spi = NULL;
    uint8_t _cs;
    uint8_t _dc;
    uint8_t _rst;
    uint16_t _width;
    uint16_t _height;
    ILI934X_ROTATION _rotation;
    uint16_t _init_width;
    uint16_t _init_height;
    uint8_t _init_rotation;
    uint8_t _colour_order;
};
