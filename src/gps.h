/*
 * GPS class
 *
 * (c) 2024 Erik Tkal
 *
 */

#pragma once

#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

using namespace std;

class SatInfo
{
public:
    SatInfo(uint num = 0, uint el = 0, uint az = 0, uint rssi = 0)
    {
        m_num  = num;
        m_el   = el;
        m_az   = az;
        m_rssi = rssi;
    }
    ~SatInfo()
    {
    }

    uint m_num;
    uint m_el;
    uint m_az;
    uint m_rssi;
};

typedef map<uint, SatInfo> SatList;
typedef vector<uint> UsedList;

class GPSData
{
public:
    typedef std::shared_ptr<GPSData> Shared;

    GPSData()  = default;
    ~GPSData() = default;

    string strLatitude;
    string strLongitude;
    string strAltitude;
    string strNumSats;
    string strGPSTime;
    string strMode3D;
    string strSpeedKts;
    SatList mSatList;
    UsedList vUsedList;
};

typedef void (*sentenceCallback)(void* pCtx, string strSentence);
typedef void (*gpsDataCallback)(void* pCtx, GPSData::Shared spGPSData);

class GPS
{
public:
    typedef std::shared_ptr<GPS> Shared;

    GPS(uart_inst_t* m_pUART);
    ~GPS();

    void SetSentenceCallback(void* pCtx, sentenceCallback pCB);
    void SetGpsDataCallback(void* pCtx, gpsDataCallback pCB);
    void Run();
    bool HasPosition()
    {
        return m_bFixPos;
    }
    bool ExternalAntenna()
    {
        return m_bExternalAntenna;
    }
    uart_inst_t* GetUART()
    {
        return m_pUART;
    }

private:
    void processSentence(string strSentence);
    bool validateSentence(string& strSentence);
    string checkSum(const string& strSentence);
    string convertToDegrees(string strRaw, int width);

    uart_inst_t* m_pUART;
    bool m_bFixTime;
    bool m_bFixPos;
    bool m_bExternalAntenna;
    bool m_bGSVInProgress;
    string m_strNumGSV;
    uint64_t m_nSatListTime;
    GPSData::Shared m_spGPSData;
    SatList m_mSatListPersistent;

    sentenceCallback m_pSentenceCallBack;
    void* m_pSentenceCtx;
    gpsDataCallback m_pGpsDataCallback;
    void* m_pGpsDataCtx;
};
