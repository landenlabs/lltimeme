///
// Author: Dennis Lang 2010
// https://landenlabs.com
//

///
// Simple wrapper classes to manage performance counters.
//

#pragma once

#include "PerfCounters.h"
#include <vector>

extern void Printf2(FILE* log, wchar_t* wFmt, ...);
extern void PrintFileTime2(FILE* log, wchar_t* pFmt, FILETIME ft);
extern ULARGE_INTEGER FILETIMEsub(FILETIME& num, FILETIME& minus);

//-----------------------------------------------------------------------------

class PerfItem
{
public:
    PerfItem(TCHAR* counterName) :
        m_counter(counterName), m_total(0), m_collectionCount(0)
        {}

    void Collect()
    {
        double d;
        if (m_counter.GetValue(d))
        {
            m_total = (m_counter.IsPerSec() ? m_total + d : d);
            m_collectionCount++;
        }
    }


    PerfCounters m_counter;
    double       m_total;
    size_t       m_collectionCount;
};

//-----------------------------------------------------------------------------
class TimeProcess
{
public:
    TimeProcess(wchar_t* names[], wchar_t* procName);
    ~TimeProcess();

    bool Collect();
    void Display(FILE* out, wchar_t* fmt[]);

    std::vector<PerfItem*> m_pCounters;
    DWORD           m_startTick;

private:
    TimeProcess(const TimeProcess& other);
    TimeProcess& operator=(const TimeProcess& other);
};
