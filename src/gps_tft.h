/*
 * GPS using TFT display
 *
 * (c) 2024 Erik Tkal
 *
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/uart.h>

#include <memory>

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
    typedef std::shared_ptr<GPS_TFT> Shared;

    GPS_TFT(ILI934X::Shared spDisplay, GPS::Shared spGPS, LED::Shared spLED, float GMToffset = 0.0);
    ~GPS_TFT();

    void Initialize();
    void Run();

private:
    static void sentenceCB(void* pCtx, string strSentence);
    static void gpsDataCB(void* pCtx, GPSData::Shared spGPSData);

    void updateUI(GPSData::Shared spGPSData);
    void drawSatGrid(uint xCenter, uint yCenter, uint radius, uint nRings = 3);
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
    void drawText(int nLine, string strText, uint16_t color = COLOUR_WHITE, bool bRightAlign = true, uint nPadding = 0);

    ILI934X::Shared m_spDisplay;
    GPS::Shared m_spGPS;
    LED::Shared m_spLED;
    float m_GMToffset;

    GPSData::Shared m_spGPSData;
};
