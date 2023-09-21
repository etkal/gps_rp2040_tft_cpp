/*
 * GPS using TFT display
 *
 * (c) 2023 Erik Tkal
 *
 */

#include <stdio.h>
#include <string>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "ili934x.h"
#include "gps_tft.h"
#include <pico/double.h>
#include <math.h>
#include <iomanip>

using namespace std;

#define SAT_ICON_RADIUS 4

constexpr uint CHAR_WIDTH  = 8;
constexpr uint CHAR_HEIGHT = 8;
constexpr uint LINE_HEIGHT = (CHAR_HEIGHT + 1);
constexpr uint COL_WIDTH   = (CHAR_WIDTH);
constexpr uint PAD_CHARS_X = 3;
constexpr uint PAD_CHARS_Y = 3;
constexpr uint X_PAD       = PAD_CHARS_X * CHAR_WIDTH;
constexpr uint Y_PAD       = PAD_CHARS_Y * LINE_HEIGHT;

constexpr double pi = 3.14159265359;


GPS_TFT::GPS_TFT(ILI934X* pDisplay, GPS* pGPS, LED* pLED, float GMToffset)
    : m_pDisplay(pDisplay),
      m_pGPS(pGPS),
      m_pLED(pLED),
      m_GMToffset(GMToffset),
      m_pGPSData(NULL)
{
}

GPS_TFT::~GPS_TFT()
{
    delete m_pDisplay;
    delete m_pGPS;
    delete m_pLED;
}

void GPS_TFT::init()
{
    m_pDisplay->reset();
    m_pDisplay->init();
    sleep_ms(100);
    m_pDisplay->fill(COLOUR_BLACK);
    m_pDisplay->show();

    m_pGPS->setSentenceCallback(this, sentenceCB);
    m_pGPS->setGpsDataCallback(this, gpsDataCB);
}

void GPS_TFT::run()
{
    m_pDisplay->fill(COLOUR_BLACK);
    m_pDisplay->show();
    m_pGPS->run();
}

void GPS_TFT::sentenceCB(void* pCtx, string strSentence)
{
    // printf("sentenceCB received: %s\n", strSentence.c_str());
}

void GPS_TFT::gpsDataCB(void* pCtx, GPSData* pGPSData)
{
    // printf("gpsDataCB received time: %s\n", pGPSData->strGPSTime.c_str());
    GPS_TFT* pThis = reinterpret_cast<GPS_TFT*>(pCtx);
    pThis->UpdateUI(pGPSData);
    delete pGPSData; // We must delete
}

void GPS_TFT::UpdateUI(GPSData* pGPSData)
{
    m_pGPSData = pGPSData;

    // This code assumes TFT display 320x240 RGB565
    uint16_t nWidth  = m_pDisplay->width();
    uint16_t nHeight = m_pDisplay->height();
    m_pDisplay->fill(COLOUR_BLACK);

    // Draw satellite grid, 3 rings
    drawSatGrid(X_PAD / 2, 0, nWidth / 2, nHeight, 3);

    // Draw upper right text
    drawText(3, pGPSData->strNumSats, COLOUR_WHITE, true, X_PAD);
    drawText(4, pGPSData->strMode3D, COLOUR_WHITE, true, X_PAD);

    if (m_pLED)
    {
        if (m_pGPS->hasPosition())
        {
            m_pLED->setPixel(0, m_pGPS->externalAntenna() ? led_blue : led_green);
            m_pLED->blink_ms(20);
        }
        else
        {
            m_pLED->setPixel(0, led_red);
            m_pLED->blink_ms(20);
        }
    }
    if (!pGPSData->strGPSTime.empty())
    {
        drawText(6, pGPSData->strGPSTime, COLOUR_WHITE, true, X_PAD);
    }
    if (!pGPSData->strLatitude.empty())
    {
        drawText(0, pGPSData->strLatitude, COLOUR_WHITE, true, X_PAD);
        drawText(1, pGPSData->strLongitude, COLOUR_WHITE, true, X_PAD);
        drawText(2, pGPSData->strAltitude, COLOUR_WHITE, true, X_PAD);
        // drawText(5, pGPSData->strSpeedKts, COLOUR_WHITE, true, X_PAD);
    }

    // Draw clock
    if (!pGPSData->strGPSTime.empty())
    {
        drawClock(nWidth / 2, LINE_HEIGHT * PAD_CHARS_Y, LINE_HEIGHT * 6 / 2, pGPSData->strGPSTime);
    }

    // Draw bar graph
    drawBarGraph(nWidth / 2, nHeight / 2, nWidth / 2 - X_PAD, nHeight / 2 - Y_PAD);

    m_pGPSData = NULL;

    // blit the framebuf to the display
    m_pDisplay->show();
}

void GPS_TFT::drawSatGrid(uint x, uint y, uint width, uint height, uint nRings)
{
    uint radius  = min(width, height) / 2 - CHAR_HEIGHT;
    uint xCenter = x + width / 2;
    uint yCenter = y + height / 2;
    drawSatGridRadial(xCenter, yCenter, radius, nRings);
}

void GPS_TFT::drawSatGridRadial(uint xCenter, uint yCenter, uint radius, uint nRings)
{
    for (uint i = 1; i <= nRings; ++i)
    {
        m_pDisplay->ellipse(xCenter, yCenter, radius * i / nRings, radius * i / nRings, COLOUR_WHITE);
    }

    m_pDisplay->vline(xCenter, yCenter - radius - 2, 2 * radius + 5, COLOUR_WHITE);
    m_pDisplay->hline(xCenter - radius - 2, yCenter, 2 * radius + 5, COLOUR_WHITE);
    m_pDisplay->text("N", xCenter - CHAR_WIDTH / 2, yCenter - radius - CHAR_HEIGHT, COLOUR_RED);
    // m_pDisplay->text("'", xCenter-6, yCenter-radius-CHAR_HEIGHT/2, COLOUR_RED);
    // m_pDisplay->text("`", xCenter-2, yCenter-radius-CHAR_HEIGHT/2, COLOUR_RED);

    int satRadius = SAT_ICON_RADIUS / 2;
    if (!m_pGPSData->strLatitude.empty())
    {
        satRadius = SAT_ICON_RADIUS;
    }
    for (auto oSat : m_pGPSData->vSatList)
    {
        double elrad = oSat.m_el * pi / 180;
        double azrad = oSat.m_az * pi / 180;
        drawCircleSat(xCenter, yCenter, radius, elrad, azrad, satRadius, COLOUR_WHITE, COLOUR_BLACK);
        for (auto nSat : m_pGPSData->vUsedList)
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
    m_pDisplay->ellipse(x, y, satRadius, satRadius, fillColor, true); // Clear area with fill
    m_pDisplay->ellipse(x, y, satRadius, satRadius, color);           // Draw circle without fill
}

void GPS_TFT::drawBarGraph(uint x, uint y, uint width, uint height)
{
    static const uint nMaxSats = 14;
    uint barDelta              = width / nMaxSats;
    uint barWidth              = barDelta - 2;
    uint barPosX               = x + width - (m_pGPSData->vSatList.size() * barDelta);
    uint barHeightMax          = height - LINE_HEIGHT * 2;

    for (auto oSat : m_pGPSData->vSatList)
    {
        uint rssi      = oSat.m_rssi;
        uint barHeight = (int)((double)barHeightMax * (double)rssi / 64);
        uint baseLineY = y + barHeightMax;
        m_pDisplay->hline(barPosX, baseLineY, barDelta, COLOUR_WHITE);
        uint nSat = oSat.m_num;
        std::stringstream oss;
        oss << fixed << setw(2) << setfill('0') << setprecision(0) << nSat;
        string strSatNum = oss.str();
        uint charPosX    = barPosX + (barDelta - CHAR_WIDTH) / 2;
        m_pDisplay->text(strSatNum.substr(0, 1).c_str(), charPosX, baseLineY + 2, COLOUR_WHITE);
        m_pDisplay->text(strSatNum.substr(1, 1).c_str(), charPosX, baseLineY + CHAR_HEIGHT + 2, COLOUR_WHITE);
        if (barHeight > 0)
        {
            m_pDisplay->rect(barPosX + 1, baseLineY - barHeight + 1, barWidth, barHeight, COLOUR_WHITE);
        }
        for (auto nSat : m_pGPSData->vUsedList)
        {
            if (oSat.m_num == nSat)
            {
                if (barHeight > 0)
                {
                    m_pDisplay->rect(barPosX + 2, baseLineY - barHeight + 2, barWidth - 2, barHeight - 2, COLOUR_BLUE, true);
                }
                break;
            }
        }
        barPosX += barDelta;
    }
}

void GPS_TFT::drawClock(uint x, uint y, uint radius, string strTime)
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
    m_pDisplay->ellipse(xCenter, yCenter, radius, radius, ringColor, false);
    m_pDisplay->ellipse(xCenter, yCenter, radius - 1, radius - 1, faceColor, true);
    // Draw quarter dots
    for (uint degDot = 0; degDot < 360; degDot += 30)
    {
        uint16_t colDot = (degDot % 90 == 0) ? COLOUR_BLUE : COLOUR_GRAY;
        uint16_t sizDot = (degDot % 90 == 0) ? 2 : 1;
        uint dxDot      = int((radius - sizDot) * sin(degDot * pi / 180));
        uint dyDot      = int((radius - sizDot) * -cos(degDot * pi / 180));
        m_pDisplay->ellipse(xCenter + dxDot, yCenter + dyDot, sizDot, sizDot, colDot, true);
    }
    // Draw the hands
    m_pDisplay->line(xCenter, yCenter, xCenter + dxs, yCenter + dys, secondHandColor);
    m_pDisplay->line(xCenter, yCenter, xCenter + dxh, yCenter + dyh, handColor);
    m_pDisplay->line(xCenter, yCenter, xCenter + dxm, yCenter + dym, handColor);
    // m_pDisplay->ellipse(xCenter, yCenter, 1, 1, faceColor, true);
}

int GPS_TFT::linePos(int nLine)
{
    if (nLine >= 0)
        return (nLine + PAD_CHARS_Y) * LINE_HEIGHT;
    else
        return m_pDisplay->height() + 1 + ((nLine - PAD_CHARS_Y) * LINE_HEIGHT);
}

void GPS_TFT::drawText(int nLine, string strText, uint16_t color, bool bRightAlign, uint nRightPad)
{
    int x = (!bRightAlign) ? 0 : m_pDisplay->width() - (strText.length() * COL_WIDTH);
    int y = linePos(nLine);
    x     = x - nRightPad;
    m_pDisplay->text(strText.c_str(), x, y, color);
}
