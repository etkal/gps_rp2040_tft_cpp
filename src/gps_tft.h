/*
 * GPS using TFT display
 *
 * (c) 2023 Erik Tkal
 *
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/uart.h>

#include "ili934x.h"
#include "gps.h"
#include "led.h"

// GPS_TFT class
//
// This combines an TFT display, GPS module and LED.
// The devices are initialized here and then callbacks are set up in order
// to receive data from the GPS, and the resulting data is displayed
// and the LED can be used to indicate a position lock and/or other
// status.
//
class GPS_TFT
{
public:
    GPS_TFT(ILI934X* pDisplay, GPS* pGPS, LED* pLED, float GMToffset = 0.0);
    ~GPS_TFT();

    void init();
    void run();

private:
    static void sentenceCB(void* pCtx, string strSentence);
    static void gpsDataCB(void* pCtx, GPSData* pGPSData);

    void UpdateUI(GPSData* pGPSData);
    void drawSatGrid(uint x, uint y, uint width, uint height, uint nRings = 3);
    void drawSatGridRadial(uint xCenter, uint yCenter, uint radius, uint nRings = 3);
    void drawBarGraph(uint x, uint y, uint width, uint height);
    void drawClock(uint x, uint y, uint radius, string strTime);
    void drawCircleSat(uint gridCenterX,
                       uint gridCenterY,
                       uint nGridRadius,
                       float elrad,
                       float azrad,
                       uint satRadius,
                       uint16_t color     = COLOUR_WHITE,
                       uint16_t fillColor = COLOUR_WHITE);
    int linePos(int nLine);
    void drawText(int nLine, string strText, uint16_t color = COLOUR_WHITE, bool bRightAlign = true, uint nRightPad = 0);

    ILI934X* m_pDisplay;
    GPS* m_pGPS;
    LED* m_pLED;
    float m_GMToffset;

    GPSData* m_pGPSData;
};
