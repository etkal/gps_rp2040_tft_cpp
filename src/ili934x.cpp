/*
 * Pico ILI934x TFT display driver class
 *
 * (c) 2024 Erik Tkal
 *
 * Modified from Darren Horrocks version to fix command/data/select timing, as well
 * as removing the GFXFont support.
 * Modified to inherit from Framebuffer (itself a version modified from analyzing
 * the Damien P. George MicroPython modframebuf.c).
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

#include "stdlib.h"
#include "stdio.h"
#include <iostream>
#include <cstring>
#include <vector>

#include "ili934x.h"
#include "hardware/gpio.h"

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))
#endif
#ifndef __swap_int
#define __swap_int(a, b)   \
    a = (a & b) + (a | b); \
    b = a + (~b) + 1;      \
    a = a + (~b) + 1;
#endif

// Note: cs is optional, set to 255 if not used
ILI934X::ILI934X(spi_inst_t* spi, uint8_t cs, uint8_t dc, uint8_t rst, ROTATION rotation)
    : m_spi(spi),
      m_cs(cs),
      m_dc(dc),
      m_rst(rst),
      m_dispWidth(0),
      m_dispHeight(0),
      m_rotation(rotation),
      m_madctl(COLOUR_ORDER_BGR),
      m_pFramebuf(nullptr),
      m_nQuadrants(4),
      m_eQuadrant(FULL_FRAME),
      m_xoff(0),
      m_yoff(0)
{
}

ILI934X::~ILI934X()
{
    if (nullptr != m_pFramebuf)
    {
        delete m_pFramebuf;
    }
}

ILI948X::ILI948X(spi_inst_t* spi, uint8_t cs, uint8_t dc, uint8_t rst, ROTATION rotation)
    : ILI934X(spi, cs, dc, rst, rotation)
{
}

void ILI934X::Reset()
{
    gpio_put(m_rst, 1);
    sleep_ms(50);
    gpio_put(m_rst, 0);
    sleep_ms(50);
    gpio_put(m_rst, 1);
    sleep_ms(50);
}

void ILI934X::Initialize()
{
    setRotation(ILI934X_HW_WIDTH, ILI934X_HW_HEIGHT, m_rotation); // Sets width, height and MADCTL value
    createFramebuf();

    // Reset the display
    Reset();

    // Set the registers
    _write(_RDDSDR, (uint8_t*)"\x03\x80\x02", 3);
    _write(_PWCRTLB, (uint8_t*)"\x00\xc1\x30", 3);
    _write(_PWRONCTRL, (uint8_t*)"\x64\x03\x12\x81", 4);
    _write(_DTCTRLA, (uint8_t*)"\x85\x00\x78", 3);
    _write(_PWCTRLA, (uint8_t*)"\x39\x2c\x00\x34\x02", 5);
    _write(_PRCTRL, (uint8_t*)"\x20", 1);
    _write(_DTCTRLB, (uint8_t*)"\x00\x00", 2);
    _write(_PWCTRL1, (uint8_t*)"\x23", 1);
    _write(_PWCTRL2, (uint8_t*)"\x10", 1);
    _write(_VMCTRL1, (uint8_t*)"\x3e\x28", 2);
    _write(_VMCTRL2, (uint8_t*)"\x86", 1);
    _write(_MADCTL, &m_madctl, 1);
    _write(_PIXSET, (uint8_t*)"\x55", 1);
    _write(_FRMCTR1, (uint8_t*)"\x00\x18", 2);
    _write(_DISCTRL, (uint8_t*)"\x08\x82\x27", 3);
    _write(_ENA3G, (uint8_t*)"\x00", 1);
    _write(_GAMSET, (uint8_t*)"\x01", 1);
    _write(_PGAMCTRL, (uint8_t*)"\x0f\x31\x2b\x0c\x0e\x08\x4e\xf1\x37\x07\x10\x03\x0e\x09\x00", 15);
    _write(_NGAMCTRL, (uint8_t*)"\x00\x0e\x14\x03\x11\x07\x31\xc1\x48\x08\x0f\x0c\x31\x36\x0f", 15);

    _write(_SLPOUT);
    _write(_DISPON);
}

void ILI948X::Initialize()
{
    setRotation(ILI948X_HW_WIDTH, ILI948X_HW_HEIGHT, m_rotation); // Sets width, height and MADCTL value
    createFramebuf();

    // Reset the display
    Reset();

    // Set the registers

    _write(_DSPINVON);
    _write(_PWCTRL3);
    _data(0x33);
    _write(_VMCTRL1);
    _data(0x00);
    _data(0x1e);
    _data(0x80);
    _write(_FRMCTR1);
    _data(0xb0);
    _write(_PGAMCTRL);
    _data(0x00);
    _data(0x13);
    _data(0x18);
    _data(0x04);
    _data(0x0F);
    _data(0x06);
    _data(0x3a);
    _data(0x56);
    _data(0x4d);
    _data(0x03);
    _data(0x0a);
    _data(0x06);
    _data(0x30);
    _data(0x3e);
    _data(0x0f);
    _write(_NGAMCTRL);
    _data(0x00);
    _data(0x13);
    _data(0x18);
    _data(0x01);
    _data(0x11);
    _data(0x06);
    _data(0x38);
    _data(0x34);
    _data(0x4d);
    _data(0x06);
    _data(0x0d);
    _data(0x0b);
    _data(0x31);
    _data(0x37);
    _data(0x0f);
    _write(_PIXSET);
    _data(0x55);
    _write(_SLPOUT);
    sleep_ms(50);
    _write(_DISPON);
    _write(_DISCTRL);
    _data(0x00);
    _data(0x02);
    _write(_MADCTL);
    _data(m_madctl);
}

void ILI934X::Clear(uint16_t colour)
{
    std::vector<uint16_t> buf(m_dispWidth, __builtin_bswap16(colour));
    uint8_t* pData = reinterpret_cast<uint8_t*>(buf.data());
    _writeBlock(0, 0, m_dispWidth - 1, m_dispHeight - 1);
    for (uint16_t iy = 0; iy < m_dispHeight; ++iy)
    {
        _data(pData, m_dispWidth * 2);
    }
}

void ILI934X::SetQuadrant(QUADRANT eQuadrant)
{
    m_eQuadrant = eQuadrant;
    switch (m_eQuadrant)
    {
    case FULL_FRAME:
    case LEFT_HALF:
    case UPPER_HALF:
    case UPPER_LEFT:
        m_xoff = 0;
        m_yoff = 0;
        break;
    case RIGHT_HALF:
    case UPPER_RIGHT:
        m_xoff = m_dispWidth / 2;
        m_yoff = 0;
        break;
    case LOWER_HALF:
    case LOWER_LEFT:
        m_xoff = 0;
        m_yoff = m_dispHeight / 2;
        break;
    case LOWER_RIGHT:
        m_xoff = m_dispWidth / 2;
        m_yoff = m_dispHeight / 2;
        break;
    }
}

std::list<QUADRANT> ILI934X::GetQuadrants()
{
    return quadrantList;
}

void ILI934X::setRotation(uint16_t screenWidth, uint16_t screenHeight, ROTATION rotation)
{
    switch (rotation)
    {
    case R0DEG:
        m_madctl |= MADCTL_MX;
        m_dispWidth  = screenWidth;
        m_dispHeight = screenHeight;
        break;
    case R90DEG:
        m_madctl |= MADCTL_MV;
        m_dispWidth  = screenHeight;
        m_dispHeight = screenWidth;
        break;
    case R180DEG:
        m_madctl |= MADCTL_MY;
        m_dispWidth  = screenWidth;
        m_dispHeight = screenHeight;
        break;
    case R270DEG:
        m_madctl |= (MADCTL_MY | MADCTL_MX | MADCTL_MV);
        m_dispWidth  = screenHeight;
        m_dispHeight = screenWidth;
        break;
    case MIRRORED0DEG:
        m_madctl |= MADCTL_MY | MADCTL_MX;
        m_dispWidth  = screenWidth;
        m_dispHeight = screenHeight;
        break;
    case MIRRORED90DEG:
        m_madctl |= (MADCTL_MX | MADCTL_MV);
        m_dispWidth  = screenHeight;
        m_dispHeight = screenWidth;
        break;
    case MIRRORED180DEG:
        m_dispWidth  = screenWidth;
        m_dispHeight = screenHeight;
        break;
    case MIRRORED270DEG:
        m_madctl |= (MADCTL_MY | MADCTL_MV);
        m_dispWidth  = screenHeight;
        m_dispHeight = screenWidth;
        break;
    }
}

void ILI934X::createFramebuf()
{
    m_pFramebuf = new Framebuf();
    switch (m_nQuadrants)
    {
    case 1:
        m_pFramebuf->Initialize(m_dispWidth, m_dispHeight, RGB565, bReverseBytes);
        quadrantList = {FULL_FRAME};
        break;
    case 2:
        if (m_dispWidth > m_dispHeight)
        {
            m_pFramebuf->Initialize(m_dispWidth / 2, m_dispHeight, RGB565, bReverseBytes);
            quadrantList = {LEFT_HALF, RIGHT_HALF};
        }
        else
        {
            m_pFramebuf->Initialize(m_dispWidth, m_dispHeight / 2, RGB565, bReverseBytes);
            quadrantList = {UPPER_HALF, LOWER_HALF};
        }
        break;
    case 4:
        m_pFramebuf->Initialize(m_dispWidth / 2, m_dispHeight / 2, RGB565, bReverseBytes);
        quadrantList = {UPPER_LEFT, LOWER_LEFT, UPPER_RIGHT, LOWER_RIGHT};
        break;
    default:
        break;
    }
}

void ILI934X::SetPixel(int x, int y, uint16_t color)
{
    adjustPoint(x, y);
    return m_pFramebuf->setpixel(x, y, color);
}

uint16_t ILI934X::GetPixel(int x, int y)
{
    adjustPoint(x, y);
    return m_pFramebuf->getpixel(x, y);
}

void ILI934X::FillRect(int x, int y, int w, int h, uint16_t color)
{
    adjustPoint(x, y);
    return m_pFramebuf->fillrect(x, y, w, h, color);
}

void ILI934X::Fill(uint16_t color)
{
    return m_pFramebuf->fill(color);
}

void ILI934X::HLine(int x, int y, int w, uint16_t color)
{
    adjustPoint(x, y);
    return m_pFramebuf->hline(x, y, w, color);
}

void ILI934X::VLine(int x, int y, int h, uint16_t color)
{
    adjustPoint(x, y);
    return m_pFramebuf->vline(x, y, h, color);
}

void ILI934X::Rect(int x, int y, int w, int h, uint16_t color, bool bFill)
{
    adjustPoint(x, y);
    return m_pFramebuf->rect(x, y, w, h, color, bFill);
}

void ILI934X::Line(int x1, int y1, int x2, int y2, uint16_t color)
{
    adjustPoint(x1, y1);
    adjustPoint(x2, y2);
    return m_pFramebuf->line(x1, y1, x2, y2, color);
}

void ILI934X::Ellipse(int cx, int cy, int xradius, int yradius, uint16_t color, bool bFill, uint8_t mask)
{
    adjustPoint(cx, cy);
    return m_pFramebuf->ellipse(cx, cy, xradius, yradius, color, bFill, mask);
}

void ILI934X::Text(const char* str, int x, int y, uint16_t color)
{
    adjustPoint(x, y);
    return m_pFramebuf->text(str, x, y, color);
}

void ILI934X::Show()
{
    Show(0, 0, m_pFramebuf->width(), m_pFramebuf->height());
}

void ILI934X::Show(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t disp_x = x + m_xoff;
    uint16_t disp_y = y + m_yoff;

    uint16_t _x = MIN(m_pFramebuf->width() - 1, MAX(0, x));
    uint16_t _y = MIN(m_pFramebuf->height() - 1, MAX(0, y));
    uint16_t _w = MIN(m_pFramebuf->width() - x, MAX(1, w));
    uint16_t _h = MIN(m_pFramebuf->height() - y, MAX(1, h));

    uint8_t* pSrcData8 = reinterpret_cast<uint8_t*>(m_pFramebuf->buffer());
    uint16_t fWidth    = m_pFramebuf->width(); // framebuf width

    // This is the simplest, gets ~15fps.
    // _writeBlock(_x, _y, _x + _w - 1, _y + _h - 1);
    // for (uint16_t iy = _y; iy < _y + _h; ++iy) // Draw line by line
    // {
    //     _data(&pSrcData8[(iy * fWidth * 2) + _x], _w * 2);
    // }

    // This is more complicated, gets ~19fps.
    static uint16_t tgtBuffer[_MAX_CHUNK_SIZE];
    uint8_t* pTgtData8     = reinterpret_cast<uint8_t*>(tgtBuffer);
    uint16_t linesPerChunk = _MAX_CHUNK_SIZE / _w;
    uint16_t numChunks     = _h / linesPerChunk;
    uint16_t linesLeftover = _h - numChunks * linesPerChunk;

    _writeBlock(disp_x, disp_y, disp_x + _w - 1, disp_y + _h - 1);
    for (uint16_t nChunk = 0; nChunk < numChunks; ++nChunk)
    {
        for (uint16_t iy = 0; iy < linesPerChunk; ++iy)
        {
            uint nSrcOffset = ((_y + (iy + nChunk * linesPerChunk)) * fWidth * 2) + (_x * 2);
            memcpy(pTgtData8 + iy * _w * 2, &pSrcData8[nSrcOffset], _w * 2);
        }
        _data(pTgtData8, linesPerChunk * _w * 2);
    }
    // Leftover lines
    for (uint16_t iy = 0; iy < linesLeftover; ++iy)
    {
        uint nSrcOffset = ((_y + (iy + numChunks * linesPerChunk)) * fWidth * 2) + (_x * 2);
        memcpy(pTgtData8 + iy * _w * 2, &pSrcData8[nSrcOffset], _w * 2);
    }
    _data(pTgtData8, linesLeftover * _w * 2);
}

void ILI934X::_writeByte(uint8_t data)
{
    spi_write_blocking(m_spi, &data, 1);
}

void ILI934X::_write(uint8_t cmd, uint8_t* data, size_t dataLen)
{
    _cs_select();
    _command_select();

    // spi write
    uint8_t commandBuffer[1];
    commandBuffer[0] = cmd;

    while (!spi_is_writable(m_spi))
    {
        sleep_us(1);
    }

    spi_write_blocking(m_spi, commandBuffer, 1);

    if (data == NULL)
    {
        _cs_deselect();
    }

    // do stuff
    if (data != NULL)
    {
        _data(data, dataLen);
    }
}

void ILI934X::_data(uint16_t data)
{
    _cs_select();
    _data_select();

    _writeByte(data & 0xff);

    _cs_deselect();
}

void ILI948X::_data(uint16_t data)
{
    _cs_select();
    _data_select();

    _writeByte(data >> 8);
    _writeByte(data & 0xff);

    _cs_deselect();
}

void ILI934X::_data(uint8_t* data, size_t dataLen)
{
    _cs_select();
    _data_select();

    spi_write_blocking(m_spi, data, dataLen);

    _cs_deselect();
}

void ILI934X::_writeBlock(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t* data, size_t dataLen)
{
    uint16_t buffer[2];
    uint8_t* pBuffer = reinterpret_cast<uint8_t*>(buffer);

    buffer[0] = __builtin_bswap16(x0);
    buffer[1] = __builtin_bswap16(x1);

    _write(_CASET);
    _data(pBuffer[0]);
    _data(pBuffer[1]);
    _data(pBuffer[2]);
    _data(pBuffer[3]);

    buffer[0] = __builtin_bswap16(y0);
    buffer[1] = __builtin_bswap16(y1);

    _write(_PASET);
    _data(pBuffer[0]);
    _data(pBuffer[1]);
    _data(pBuffer[2]);
    _data(pBuffer[3]);

    _write(_RAMWR, data, dataLen);
}
