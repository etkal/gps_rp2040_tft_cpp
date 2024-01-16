/*
 * Pico ILI TFT display driver class
 *
 * Currently supports ILI9341 and ILI9488 displays.
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

#include "ili_tft.h"
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

ILI_TFT::ILI_TFT(spi_inst_t* spi, uint8_t cs, uint8_t dc, uint8_t rst, ROTATION rotation)
    : m_spi(spi),
      m_cs(cs),
      m_dc(dc),
      m_rst(rst),
      m_dispWidth(0),
      m_dispHeight(0),
      m_rotation(rotation),
      m_madctl(COLOUR_ORDER_BGR),
      m_spFramebuf(nullptr),
      m_nQuadrants(DISPLAY_QUADRANTS),
      m_eQuadrant(FULL_FRAME),
      m_xoff(0),
      m_yoff(0)
{
}

ILI934X::ILI934X(spi_inst_t* spi, uint8_t cs, uint8_t dc, uint8_t rst, ROTATION rotation)
    : ILI_TFT(spi, cs, dc, rst, rotation)
{
}

ILI948X::ILI948X(spi_inst_t* spi, uint8_t cs, uint8_t dc, uint8_t rst, ROTATION rotation)
    : ILI_TFT(spi, cs, dc, rst, rotation)
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

void ILI948X::Reset()
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
    write(_RDDSDR, (uint8_t*)"\x03\x80\x02", 3);
    write(_PWCRTLB, (uint8_t*)"\x00\xc1\x30", 3);
    write(_PWRONCTRL, (uint8_t*)"\x64\x03\x12\x81", 4);
    write(_DTCTRLA, (uint8_t*)"\x85\x00\x78", 3);
    write(_PWCTRLA, (uint8_t*)"\x39\x2c\x00\x34\x02", 5);
    write(_PRCTRL, (uint8_t*)"\x20", 1);
    write(_DTCTRLB, (uint8_t*)"\x00\x00", 2);
    write(_PWCTRL1, (uint8_t*)"\x23", 1);
    write(_PWCTRL2, (uint8_t*)"\x10", 1);
    write(_VMCTRL1, (uint8_t*)"\x3e\x28", 2);
    write(_VMCTRL2, (uint8_t*)"\x86", 1);
    write(_MADCTL, &m_madctl, 1);
    write(_PIXSET, (uint8_t*)"\x55", 1);
    write(_FRMCTR1, (uint8_t*)"\x00\x18", 2);
    write(_DISCTRL, (uint8_t*)"\x08\x82\x27", 3);
    write(_ENA3G, (uint8_t*)"\x00", 1);
    write(_GAMSET, (uint8_t*)"\x01", 1);
    write(_PGAMCTRL, (uint8_t*)"\x0f\x31\x2b\x0c\x0e\x08\x4e\xf1\x37\x07\x10\x03\x0e\x09\x00", 15);
    write(_NGAMCTRL, (uint8_t*)"\x00\x0e\x14\x03\x11\x07\x31\xc1\x48\x08\x0f\x0c\x31\x36\x0f", 15);

    write(_SLPOUT);
    write(_DISPON);
}

void ILI948X::Initialize()
{
    setRotation(ILI948X_HW_WIDTH, ILI948X_HW_HEIGHT, m_rotation); // Sets width, height and MADCTL value
    createFramebuf();

    // Reset the display
    Reset();

    // Set the registers
    write(_DSPINVON);
    write(_PWCTRL3);
    sendData(0x33);
    write(_VMCTRL1);
    sendData(0x00);
    sendData(0x1e);
    sendData(0x80);
    write(_FRMCTR1);
    sendData(0xb0);
    write(_PGAMCTRL);
    sendData(0x00);
    sendData(0x13);
    sendData(0x18);
    sendData(0x04);
    sendData(0x0F);
    sendData(0x06);
    sendData(0x3a);
    sendData(0x56);
    sendData(0x4d);
    sendData(0x03);
    sendData(0x0a);
    sendData(0x06);
    sendData(0x30);
    sendData(0x3e);
    sendData(0x0f);
    write(_NGAMCTRL);
    sendData(0x00);
    sendData(0x13);
    sendData(0x18);
    sendData(0x01);
    sendData(0x11);
    sendData(0x06);
    sendData(0x38);
    sendData(0x34);
    sendData(0x4d);
    sendData(0x06);
    sendData(0x0d);
    sendData(0x0b);
    sendData(0x31);
    sendData(0x37);
    sendData(0x0f);
    write(_PIXSET);
    sendData(0x55);
    write(_SLPOUT);
    sleep_ms(50);
    write(_DISPON);
    write(_DISCTRL);
    sendData(0x00);
    sendData(0x02);
    write(_MADCTL);
    sendData(m_madctl);
}

void ILI_TFT::Clear(uint16_t colour)
{
    std::vector<uint16_t> buf(m_dispWidth, __builtin_bswap16(colour));
    uint8_t* pData = reinterpret_cast<uint8_t*>(buf.data());
    writeBlock(0, 0, m_dispWidth - 1, m_dispHeight - 1);
    for (uint16_t iy = 0; iy < m_dispHeight; ++iy)
    {
        sendData(pData, m_dispWidth * 2);
    }
}

void ILI_TFT::SetQuadrant(QUADRANT eQuadrant)
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

std::list<QUADRANT> ILI_TFT::GetQuadrants()
{
    return quadrantList;
}

void ILI_TFT::setRotation(uint16_t screenWidth, uint16_t screenHeight, ROTATION rotation)
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

void ILI_TFT::createFramebuf()
{
    m_spFramebuf = std::make_shared<Framebuf>();
    switch (m_nQuadrants)
    {
    case 1:
        m_spFramebuf->Initialize(m_dispWidth, m_dispHeight, RGB565, bReverseBytes);
        quadrantList = {FULL_FRAME};
        break;
    case 2:
        if (m_dispWidth > m_dispHeight)
        {
            m_spFramebuf->Initialize(m_dispWidth / 2, m_dispHeight, RGB565, bReverseBytes);
            quadrantList = {LEFT_HALF, RIGHT_HALF};
        }
        else
        {
            m_spFramebuf->Initialize(m_dispWidth, m_dispHeight / 2, RGB565, bReverseBytes);
            quadrantList = {UPPER_HALF, LOWER_HALF};
        }
        break;
    case 4:
        m_spFramebuf->Initialize(m_dispWidth / 2, m_dispHeight / 2, RGB565, bReverseBytes);
        quadrantList = {UPPER_LEFT, LOWER_LEFT, UPPER_RIGHT, LOWER_RIGHT};
        break;
    default:
        break;
    }
}

void ILI_TFT::SetPixel(int x, int y, uint16_t color)
{
    adjustPoint(x, y);
    return m_spFramebuf->setpixel(x, y, color);
}

uint16_t ILI_TFT::GetPixel(int x, int y)
{
    adjustPoint(x, y);
    return m_spFramebuf->getpixel(x, y);
}

void ILI_TFT::FillRect(int x, int y, int w, int h, uint16_t color)
{
    adjustPoint(x, y);
    return m_spFramebuf->fillrect(x, y, w, h, color);
}

void ILI_TFT::Fill(uint16_t color)
{
    return m_spFramebuf->fill(color);
}

void ILI_TFT::HLine(int x, int y, int w, uint16_t color)
{
    adjustPoint(x, y);
    return m_spFramebuf->hline(x, y, w, color);
}

void ILI_TFT::VLine(int x, int y, int h, uint16_t color)
{
    adjustPoint(x, y);
    return m_spFramebuf->vline(x, y, h, color);
}

void ILI_TFT::Rect(int x, int y, int w, int h, uint16_t color, bool bFill)
{
    adjustPoint(x, y);
    return m_spFramebuf->rect(x, y, w, h, color, bFill);
}

void ILI_TFT::Line(int x1, int y1, int x2, int y2, uint16_t color)
{
    adjustPoint(x1, y1);
    adjustPoint(x2, y2);
    return m_spFramebuf->line(x1, y1, x2, y2, color);
}

void ILI_TFT::Ellipse(int cx, int cy, int xradius, int yradius, uint16_t color, bool bFill, uint8_t mask)
{
    adjustPoint(cx, cy);
    return m_spFramebuf->ellipse(cx, cy, xradius, yradius, color, bFill, mask);
}

void ILI_TFT::Text(const char* str, int x, int y, uint16_t color)
{
    adjustPoint(x, y);
    return m_spFramebuf->text(str, x, y, color);
}

void ILI_TFT::Show()
{
    Show(0, 0, m_spFramebuf->width(), m_spFramebuf->height());
}

void ILI_TFT::Show(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t disp_x = x + m_xoff;
    uint16_t disp_y = y + m_yoff;

    uint16_t _x = MIN(m_spFramebuf->width() - 1, MAX(0, x));
    uint16_t _y = MIN(m_spFramebuf->height() - 1, MAX(0, y));
    uint16_t _w = MIN(m_spFramebuf->width() - x, MAX(1, w));
    uint16_t _h = MIN(m_spFramebuf->height() - y, MAX(1, h));

    uint8_t* pSrcData8 = reinterpret_cast<uint8_t*>(m_spFramebuf->buffer());
    uint16_t fWidth    = m_spFramebuf->width(); // framebuf width

    // This is the simplest, gets ~15fps.
    // writeBlock(_x, _y, _x + _w - 1, _y + _h - 1);
    // for (uint16_t iy = _y; iy < _y + _h; ++iy) // Draw line by line
    // {
    //     data(&pSrcData8[(iy * fWidth * 2) + _x], _w * 2);
    // }

    // This is more complicated, gets ~19fps.
    static uint16_t tgtBuffer[_MAX_CHUNK_SIZE];
    uint8_t* pTgtData8     = reinterpret_cast<uint8_t*>(tgtBuffer);
    uint16_t linesPerChunk = _MAX_CHUNK_SIZE / _w;
    uint16_t numChunks     = _h / linesPerChunk;
    uint16_t linesLeftover = _h - numChunks * linesPerChunk;

    writeBlock(disp_x, disp_y, disp_x + _w - 1, disp_y + _h - 1);
    for (uint16_t nChunk = 0; nChunk < numChunks; ++nChunk)
    {
        for (uint16_t iy = 0; iy < linesPerChunk; ++iy)
        {
            uint nSrcOffset = ((_y + (iy + nChunk * linesPerChunk)) * fWidth * 2) + (_x * 2);
            memcpy(pTgtData8 + iy * _w * 2, &pSrcData8[nSrcOffset], _w * 2);
        }
        sendData(pTgtData8, linesPerChunk * _w * 2);
    }
    // Leftover lines
    for (uint16_t iy = 0; iy < linesLeftover; ++iy)
    {
        uint nSrcOffset = ((_y + (iy + numChunks * linesPerChunk)) * fWidth * 2) + (_x * 2);
        memcpy(pTgtData8 + iy * _w * 2, &pSrcData8[nSrcOffset], _w * 2);
    }
    sendData(pTgtData8, linesLeftover * _w * 2);
}

void ILI_TFT::writeByte(uint8_t data)
{
    spi_write_blocking(m_spi, &data, 1);
}

void ILI_TFT::write(uint8_t cmd, uint8_t* data, size_t dataLen)
{
    cs_select();
    command_select();

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
        cs_deselect();
    }

    // do stuff
    if (data != NULL)
    {
        sendData(data, dataLen);
    }
}

// ILI934X sends one byte of data
void ILI934X::sendData(uint16_t data)
{
    cs_select();
    data_select();

    writeByte(data & 0xff);

    cs_deselect();
}

// ILI948X sends two bytes of data
void ILI948X::sendData(uint16_t data)
{
    cs_select();
    data_select();

    writeByte(data >> 8);
    writeByte(data & 0xff);

    cs_deselect();
}

void ILI_TFT::sendData(uint8_t* data, size_t dataLen)
{
    cs_select();
    data_select();

    spi_write_blocking(m_spi, data, dataLen);

    cs_deselect();
}

void ILI_TFT::writeBlock(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t* data, size_t dataLen)
{
    uint16_t buffer[2];
    uint8_t* pBuffer = reinterpret_cast<uint8_t*>(buffer);

    buffer[0] = __builtin_bswap16(x0);
    buffer[1] = __builtin_bswap16(x1);

    write(_CASET);
    sendData(pBuffer[0]);
    sendData(pBuffer[1]);
    sendData(pBuffer[2]);
    sendData(pBuffer[3]);

    buffer[0] = __builtin_bswap16(y0);
    buffer[1] = __builtin_bswap16(y1);

    write(_PASET);
    sendData(pBuffer[0]);
    sendData(pBuffer[1]);
    sendData(pBuffer[2]);
    sendData(pBuffer[3]);

    write(_RAMWR, data, dataLen);
}
