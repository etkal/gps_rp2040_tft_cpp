/*
 * GPS using TFT display
 *
 * (c) 2024 Erik Tkal
 *
 */

#include <stdio.h>
#include <string>
#include <iostream>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "ili_tft.h"
#include "gps_tft.h"
#include <pico/double.h>
#include <math.h>
#include <iomanip>

#if !defined(NDEBUG)
#include <malloc.h>
static uint32_t getTotalHeap()
{
    extern char __StackLimit, __bss_end__;
    return &__StackLimit - &__bss_end__;
}
static uint32_t getFreeHeap()
{
    struct mallinfo m = mallinfo();
    return getTotalHeap() - m.uordblks;
}
#endif

#define SAT_ICON_RADIUS 4

constexpr uint CHAR_WIDTH  = 8;
constexpr uint CHAR_HEIGHT = 8;
constexpr uint LINE_HEIGHT = (CHAR_HEIGHT + 1);
constexpr uint COL_WIDTH   = (CHAR_WIDTH);
constexpr uint PAD_CHARS_X = 2;
constexpr uint PAD_CHARS_Y = 2;
constexpr uint X_PAD       = PAD_CHARS_X * CHAR_WIDTH;
constexpr uint Y_PAD       = PAD_CHARS_Y * LINE_HEIGHT;

constexpr double pi = 3.14159265359;

GPS_TFT::GPS_TFT(ILI_TFT::Shared spDisplay, GPS::Shared spGPS, LED::Shared spLED, float GMToffset)
    : m_spDisplay(spDisplay),
      m_spGPS(spGPS),
      m_spLED(spLED),
      m_GMToffset(GMToffset)
{
}

GPS_TFT::~GPS_TFT()
{
}

void GPS_TFT::Initialize()
{
    m_spDisplay->Reset();
    m_spDisplay->Initialize();

    m_spDisplay->Clear(COLOUR_BLACK);

    m_spDisplay->SetQuadrant(m_spDisplay->GetQuadrants().front());
    m_spDisplay->Fill(COLOUR_BLACK);
    drawText(0, "Waiting for GPS", COLOUR_WHITE, false, 0);
    m_spDisplay->Show();

    m_spGPS->SetSentenceCallback(this, sentenceCB);
    m_spGPS->SetGpsDataCallback(this, gpsDataCB);
}

void GPS_TFT::Run()
{
    m_spGPS->Run();
}

void GPS_TFT::sentenceCB(void* pCtx, std::string strSentence)
{
    // printf("sentenceCB received: %s\n", strSentence.c_str());
}

void GPS_TFT::gpsDataCB(void* pCtx, GPSData::Shared spGPSData)
{
    GPS_TFT* pThis = reinterpret_cast<GPS_TFT*>(pCtx);
    pThis->updateUI(spGPSData);
}

void GPS_TFT::updateUI(GPSData::Shared spGPSData)
{
    m_spGPSData = spGPSData;
    if (m_spLED)
    {
        if (m_spGPS->HasPosition())
        {
            m_spLED->SetPixel(0, m_spGPS->ExternalAntenna() ? led_blue : led_green);
            m_spLED->Blink_ms(20);
        }
        else
        {
            m_spLED->SetPixel(0, led_red);
            m_spLED->Blink_ms(20);
        }
    }

    uint16_t nWidth  = m_spDisplay->Width();
    uint16_t nHeight = m_spDisplay->Height();

    for (auto nQuadrant : m_spDisplay->GetQuadrants())
    {
        m_spDisplay->SetQuadrant(nQuadrant);
        m_spDisplay->Fill(COLOUR_BLACK);

        // Draw the satellite map on the left half of the screen
        // Draw satellite grid, 3 rings
        if (m_spDisplay->Width() > m_spDisplay->Height())
        {
            drawSatGrid(nWidth / 4 + X_PAD, nHeight / 2, nWidth / 4 - X_PAD / 2, 3);
        }
        else
        {
            drawSatGrid(nWidth / 3 + X_PAD, nHeight / 2, nWidth / 3 - X_PAD / 2, 3);
        }

        // Draw upper right text
        drawText(3, spGPSData->strNumSats, COLOUR_WHITE, true, X_PAD);
        drawText(4, spGPSData->strMode3D, COLOUR_WHITE, true, X_PAD);

        if (!spGPSData->strGPSTime.empty())
        {
            drawText(6, spGPSData->strGPSTime, COLOUR_WHITE, true, X_PAD);
        }
        if (!spGPSData->strLatitude.empty())
        {
            drawText(0, spGPSData->strLatitude, COLOUR_WHITE, true, X_PAD);
            drawText(1, spGPSData->strLongitude, COLOUR_WHITE, true, X_PAD);
            drawText(2, spGPSData->strAltitude, COLOUR_WHITE, true, X_PAD);
            // drawText(5, spGPSData->strSpeedKts, COLOUR_WHITE, true, X_PAD);
        }

        // Draw clock
        if (!spGPSData->strGPSTime.empty())
        {
            uint radius = m_spDisplay->Width() <= 320 ? LINE_HEIGHT * 6 / 2 : LINE_HEIGHT * 10 / 2;
            uint xPos   = m_spDisplay->Width() > m_spDisplay->Height() ? nWidth / 2 : nWidth / 3;
            drawClock(xPos, LINE_HEIGHT * PAD_CHARS_Y, radius, spGPSData->strGPSTime);
        }

        // Draw bar graph
        if (m_spGPSData->mSatList.size() > 0)
        {
            uint xPos   = nWidth / 2;
            uint yPos   = m_spDisplay->Width() > m_spDisplay->Height() ? nHeight / 2 : nHeight * 3 / 4;
            uint width  = nWidth / 2 - X_PAD;
            uint height = m_spDisplay->Width() > m_spDisplay->Height() ? nHeight / 2 - Y_PAD : nHeight / 4 - Y_PAD;
            drawBarGraph(xPos, yPos, width, height);
        }

        // blit the framebuf to the display quadrant
        m_spDisplay->Show();
    }

    m_spGPSData.reset();

#if !defined(NDEBUG)
    std::cout << "Total Heap: " << getTotalHeap() << "  Free Heap: " << getFreeHeap() << std::endl;
#endif
}

void GPS_TFT::drawSatGrid(uint xCenter, uint yCenter, uint radius, uint nRings)
{
    for (uint i = 1; i <= nRings; ++i)
    {
        m_spDisplay->Ellipse(xCenter, yCenter, radius * i / nRings, radius * i / nRings, COLOUR_WHITE);
    }

    m_spDisplay->VLine(xCenter, yCenter - radius - 2, 2 * radius + 5, COLOUR_WHITE);
    m_spDisplay->HLine(xCenter - radius - 2, yCenter, 2 * radius + 5, COLOUR_WHITE);
    m_spDisplay->Text("N", xCenter - CHAR_WIDTH / 2, yCenter - radius - CHAR_HEIGHT, COLOUR_RED);
    // m_spDisplay->Text("'", xCenter-6, yCenter-radius-CHAR_HEIGHT/2, COLOUR_RED);
    // m_spDisplay->Text("`", xCenter-2, yCenter-radius-CHAR_HEIGHT/2, COLOUR_RED);

    int satRadius = SAT_ICON_RADIUS / 2;
    if (!m_spGPSData->strLatitude.empty())
    {
        satRadius = SAT_ICON_RADIUS;
    }
    for (auto oEntry : m_spGPSData->mSatList)
    {
        auto oSat    = oEntry.second;
        double elrad = oSat.m_el * pi / 180;
        double azrad = oSat.m_az * pi / 180;
        drawCircleSat(xCenter, yCenter, radius, elrad, azrad, satRadius, COLOUR_WHITE, COLOUR_BLACK);
        for (auto nSat : m_spGPSData->vUsedList)
        {
            if (oSat.m_num == nSat)
            {
                drawCircleSat(xCenter, yCenter, radius, elrad, azrad, satRadius, COLOUR_WHITE, COLOUR_BLUE);
                break;
            }
        }
    }
}

void GPS_TFT::drawCircleSat(uint gridCenterX,
                            uint gridCenterY,
                            uint nGridRadius,
                            float elrad,
                            float azrad,
                            uint satRadius,
                            uint16_t color,
                            uint16_t fillColor)
{
    // Draw satellite (fill first, then draw open circle)
    int dx = (nGridRadius - SAT_ICON_RADIUS) * cos(elrad) * sin(azrad);
    int dy = (nGridRadius - SAT_ICON_RADIUS) * cos(elrad) * -cos(azrad);
    int x  = gridCenterX + dx;
    int y  = gridCenterY + dy;
    m_spDisplay->Ellipse(x, y, satRadius, satRadius, fillColor, true); // Clear area with fill
    m_spDisplay->Ellipse(x, y, satRadius, satRadius, color);           // Draw circle without fill
}

void GPS_TFT::drawBarGraph(uint x, uint y, uint width, uint height)
{
    constexpr auto nMaxSats = 16;
    uint barDelta           = std::max(std::min(CHAR_WIDTH + 4, width / nMaxSats), CHAR_WIDTH);
    uint barWidth           = barDelta - 2;
    uint barPosX            = x + width - (m_spGPSData->mSatList.size() * barDelta);
    uint barHeightMax       = height - LINE_HEIGHT * 2;

    for (auto oEntry : m_spGPSData->mSatList)
    {
        auto oSat      = oEntry.second;
        uint rssi      = oSat.m_rssi;
        uint barHeight = (int)((double)barHeightMax * (double)rssi / 64);
        uint baseLineY = y + barHeightMax;
        m_spDisplay->HLine(barPosX, baseLineY, barDelta, COLOUR_WHITE);
        uint nSat = oSat.m_num;
        std::stringstream oss;
        oss << std::fixed << std::setw(2) << std::setfill('0') << std::setprecision(0) << nSat;
        std::string strSatNum = oss.str();
        uint charPosX         = barPosX + (barDelta - CHAR_WIDTH) / 2;
        m_spDisplay->Text(strSatNum.substr(0, 1).c_str(), charPosX, baseLineY + 2, COLOUR_WHITE);
        m_spDisplay->Text(strSatNum.substr(1, 1).c_str(), charPosX, baseLineY + CHAR_HEIGHT + 2, COLOUR_WHITE);
        if (barHeight > 0)
        {
            m_spDisplay->Rect(barPosX + 1, baseLineY - barHeight + 1, barWidth, barHeight, COLOUR_WHITE);
        }
        for (auto nSat : m_spGPSData->vUsedList)
        {
            if (oSat.m_num == nSat)
            {
                if (barHeight > 0)
                {
                    m_spDisplay->Rect(barPosX + 2, baseLineY - barHeight + 2, barWidth - 2, barHeight - 2, COLOUR_BLUE, true);
                }
                break;
            }
        }
        barPosX += barDelta;
    }
}

void GPS_TFT::drawClock(uint x, uint y, uint radius, std::string strTime)
{
    uint xCenter             = x + radius;
    uint yCenter             = y + radius;
    uint nHour               = atoi(strTime.substr(0, 2).c_str());
    float hour               = (float)(nHour % 12) + m_GMToffset;
    hour                     = (hour < 0) ? hour + 12 : hour;
    hour                     = (hour >= 12) ? hour - 12 : hour;
    float minute             = (float)atoi(strTime.substr(3, 2).c_str());
    float second             = (float)atoi(strTime.substr(6, 2).c_str());
    uint16_t ringColor       = COLOUR_GREEN;
    uint16_t faceColor       = COLOUR_BLACK;
    uint16_t handColor       = COLOUR_WHITE;
    uint16_t secondHandColor = COLOUR_RED;
    double handLenHour       = radius * 0.4;
    double handLenMinute     = radius * 0.7;
    double handLenSecond     = radius * 0.8;
    double radiansHour       = 2 * pi * (((hour * 3600.0) + (minute * 60.0) + second) / (12.0 * 60.0 * 60.0));
    double radiansMinute     = 2 * pi * (((minute * 60.0) + second) / (60.0 * 60.0));
    double radiansSecond     = 2 * pi * (second / 60.0);
    int dxh                  = int(handLenHour * sin(radiansHour));
    int dyh                  = int(handLenHour * -cos(radiansHour));
    int dxm                  = int(handLenMinute * sin(radiansMinute));
    int dym                  = int(handLenMinute * -cos(radiansMinute));
    int dxs                  = int(handLenSecond * sin(radiansSecond));
    int dys                  = int(handLenSecond * -cos(radiansSecond));

    // Draw the face
    m_spDisplay->Ellipse(xCenter, yCenter, radius, radius, ringColor, false);
    m_spDisplay->Ellipse(xCenter, yCenter, radius - 1, radius - 1, faceColor, true);
    // Draw quarter dots
    for (uint degDot = 0; degDot < 360; degDot += 30)
    {
        uint16_t colDot = (degDot % 90 == 0) ? COLOUR_BLUE : COLOUR_GRAY;
        uint16_t sizDot = (degDot % 90 == 0) ? 2 : 1;
        uint dxDot      = int((radius - sizDot) * sin(degDot * pi / 180));
        uint dyDot      = int((radius - sizDot) * -cos(degDot * pi / 180));
        m_spDisplay->Ellipse(xCenter + dxDot, yCenter + dyDot, sizDot, sizDot, colDot, true);
    }
    // Draw the hands
    m_spDisplay->Line(xCenter, yCenter, xCenter + dxs, yCenter + dys, secondHandColor);
    m_spDisplay->Line(xCenter, yCenter, xCenter + dxh, yCenter + dyh, handColor);
    m_spDisplay->Line(xCenter, yCenter, xCenter + dxm, yCenter + dym, handColor);
    // m_spDisplay->ellipse(xCenter, yCenter, 1, 1, faceColor, true);
}

int GPS_TFT::linePos(int nLine)
{
    if (nLine >= 0)
        return (nLine + PAD_CHARS_Y) * LINE_HEIGHT;
    else
        return m_spDisplay->Height() + 1 + ((nLine - PAD_CHARS_Y) * LINE_HEIGHT);
}

void GPS_TFT::drawText(int nLine, std::string strText, uint16_t color, bool bRightAlign, uint nPadding)
{
    int x = (!bRightAlign) ? 0 : m_spDisplay->Width() - (strText.length() * COL_WIDTH);
    int y = linePos(nLine);
    x += bRightAlign ? -nPadding : nPadding;
    m_spDisplay->Text(strText.c_str(), x, y, color);
}
