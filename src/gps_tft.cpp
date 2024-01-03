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
constexpr uint PAD_CHARS_X = 2;
constexpr uint PAD_CHARS_Y = 2;
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

void GPS_TFT::Initialize()
{
    m_pDisplay->Reset();
    m_pDisplay->Initialize();

    m_pDisplay->Clear(COLOUR_BLACK);

    m_pDisplay->SetQuadrant(m_pDisplay->GetQuadrants().front());
    m_pDisplay->Fill(COLOUR_BLACK);
    drawText(0, "Waiting for GPS", COLOUR_WHITE, false, 0);
    m_pDisplay->Show();

    m_pGPS->SetSentenceCallback(this, sentenceCB);
    m_pGPS->SetGpsDataCallback(this, gpsDataCB);
}

void GPS_TFT::Run()
{
    m_pGPS->Run();
}

void GPS_TFT::sentenceCB(void* pCtx, string strSentence)
{
    // printf("sentenceCB received: %s\n", strSentence.c_str());
}

void GPS_TFT::gpsDataCB(void* pCtx, GPSData* pGPSData)
{
    // printf("gpsDataCB received time: %s\n", pGPSData->strGPSTime.c_str());
    GPS_TFT* pThis = reinterpret_cast<GPS_TFT*>(pCtx);
    pThis->updateUI(pGPSData);
    delete pGPSData; // We must delete
}

void GPS_TFT::updateUI(GPSData* pGPSData)
{
    m_pGPSData = pGPSData;
    if (m_pLED)
    {
        if (m_pGPS->HasPosition())
        {
            m_pLED->SetPixel(0, m_pGPS->ExternalAntenna() ? led_blue : led_green);
            m_pLED->Blink_ms(20);
        }
        else
        {
            m_pLED->SetPixel(0, led_red);
            m_pLED->Blink_ms(20);
        }
    }

    uint16_t nWidth  = m_pDisplay->Width();
    uint16_t nHeight = m_pDisplay->Height();

    // Temporary, for performance measurement on quadrant display
    absolute_time_t t0, t1, t2, t3, t4;
    t0 = get_absolute_time();

    for (auto nQuadrant : m_pDisplay->GetQuadrants())
    {
        t1 = get_absolute_time();

        m_pDisplay->SetQuadrant(nQuadrant);
        m_pDisplay->Fill(COLOUR_BLACK);

        // Draw the satellite map on the left half of the screen
        // Draw satellite grid, 3 rings
        drawSatGrid(X_PAD, 0, nWidth / 2, nHeight, 3);

        // Draw upper right text
        drawText(3, pGPSData->strNumSats, COLOUR_WHITE, true, X_PAD);
        drawText(4, pGPSData->strMode3D, COLOUR_WHITE, true, X_PAD);

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
            uint radius = m_pDisplay->Width() <= 320 ? LINE_HEIGHT * 6 / 2 : LINE_HEIGHT * 10 / 2;
            drawClock(nWidth / 2, LINE_HEIGHT * PAD_CHARS_Y, radius, pGPSData->strGPSTime);
        }

        // Draw bar graph
        drawBarGraph(nWidth / 2, nHeight / 2, nWidth / 2 - X_PAD, nHeight / 2 - Y_PAD);

        t2 = get_absolute_time();

        // blit the framebuf to the display quadrant
        m_pDisplay->Show();

        t3 = get_absolute_time();
        std::cout << "Time to draw: " << absolute_time_diff_us(t1, t2) << " us" << std::endl;
        std::cout << "Time to show: " << absolute_time_diff_us(t2, t3) << " us" << std::endl;
    }
    t4 = get_absolute_time();
    std::cout << "Total frame draw time: " << absolute_time_diff_us(t0, t4) << " us" << std::endl << std::endl;
    m_pGPSData = NULL;
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
        m_pDisplay->Ellipse(xCenter, yCenter, radius * i / nRings, radius * i / nRings, COLOUR_WHITE);
    }

    m_pDisplay->VLine(xCenter, yCenter - radius - 2, 2 * radius + 5, COLOUR_WHITE);
    m_pDisplay->HLine(xCenter - radius - 2, yCenter, 2 * radius + 5, COLOUR_WHITE);
    m_pDisplay->Text("N", xCenter - CHAR_WIDTH / 2, yCenter - radius - CHAR_HEIGHT, COLOUR_RED);
    // m_pDisplay->Text("'", xCenter-6, yCenter-radius-CHAR_HEIGHT/2, COLOUR_RED);
    // m_pDisplay->Text("`", xCenter-2, yCenter-radius-CHAR_HEIGHT/2, COLOUR_RED);

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
    m_pDisplay->Ellipse(x, y, satRadius, satRadius, fillColor, true); // Clear area with fill
    m_pDisplay->Ellipse(x, y, satRadius, satRadius, color);           // Draw circle without fill
}

void GPS_TFT::drawBarGraph(uint x, uint y, uint width, uint height)
{
    constexpr auto nMaxSats = 16;
    uint barDelta           = width / nMaxSats;
    uint barWidth           = barDelta - 2;
    uint barPosX            = x + width - (m_pGPSData->vSatList.size() * barDelta);
    uint barHeightMax       = height - LINE_HEIGHT * 2;

    for (auto oSat : m_pGPSData->vSatList)
    {
        uint rssi      = oSat.m_rssi;
        uint barHeight = (int)((double)barHeightMax * (double)rssi / 64);
        uint baseLineY = y + barHeightMax;
        m_pDisplay->HLine(barPosX, baseLineY, barDelta, COLOUR_WHITE);
        uint nSat = oSat.m_num;
        std::stringstream oss;
        oss << fixed << setw(2) << setfill('0') << setprecision(0) << nSat;
        string strSatNum = oss.str();
        uint charPosX    = barPosX + (barDelta - CHAR_WIDTH) / 2;
        m_pDisplay->Text(strSatNum.substr(0, 1).c_str(), charPosX, baseLineY + 2, COLOUR_WHITE);
        m_pDisplay->Text(strSatNum.substr(1, 1).c_str(), charPosX, baseLineY + CHAR_HEIGHT + 2, COLOUR_WHITE);
        if (barHeight > 0)
        {
            m_pDisplay->Rect(barPosX + 1, baseLineY - barHeight + 1, barWidth, barHeight, COLOUR_WHITE);
        }
        for (auto nSat : m_pGPSData->vUsedList)
        {
            if (oSat.m_num == nSat)
            {
                if (barHeight > 0)
                {
                    m_pDisplay->Rect(barPosX + 2, baseLineY - barHeight + 2, barWidth - 2, barHeight - 2, COLOUR_BLUE, true);
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
    m_pDisplay->Ellipse(xCenter, yCenter, radius, radius, ringColor, false);
    m_pDisplay->Ellipse(xCenter, yCenter, radius - 1, radius - 1, faceColor, true);
    // Draw quarter dots
    for (uint degDot = 0; degDot < 360; degDot += 30)
    {
        uint16_t colDot = (degDot % 90 == 0) ? COLOUR_BLUE : COLOUR_GRAY;
        uint16_t sizDot = (degDot % 90 == 0) ? 2 : 1;
        uint dxDot      = int((radius - sizDot) * sin(degDot * pi / 180));
        uint dyDot      = int((radius - sizDot) * -cos(degDot * pi / 180));
        m_pDisplay->Ellipse(xCenter + dxDot, yCenter + dyDot, sizDot, sizDot, colDot, true);
    }
    // Draw the hands
    m_pDisplay->Line(xCenter, yCenter, xCenter + dxs, yCenter + dys, secondHandColor);
    m_pDisplay->Line(xCenter, yCenter, xCenter + dxh, yCenter + dyh, handColor);
    m_pDisplay->Line(xCenter, yCenter, xCenter + dxm, yCenter + dym, handColor);
    // m_pDisplay->ellipse(xCenter, yCenter, 1, 1, faceColor, true);
}

int GPS_TFT::linePos(int nLine)
{
    if (nLine >= 0)
        return (nLine + PAD_CHARS_Y) * LINE_HEIGHT;
    else
        return m_pDisplay->Height() + 1 + ((nLine - PAD_CHARS_Y) * LINE_HEIGHT);
}

void GPS_TFT::drawText(int nLine, string strText, uint16_t color, bool bRightAlign, uint nPadding)
{
    int x = (!bRightAlign) ? 0 : m_pDisplay->Width() - (strText.length() * COL_WIDTH);
    int y = linePos(nLine);
    x += bRightAlign ? -nPadding : nPadding;
    m_pDisplay->Text(strText.c_str(), x, y, color);
}
