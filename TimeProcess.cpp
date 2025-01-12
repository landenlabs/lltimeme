///
// Author: Dennis Lang 2010
// https://lanenlabs.com
//

///
// Simple wrapper classes to manage performance counters.
//

#include "TimeProcess.h"
#include <stdarg.h>

             
//-----------------------------------------------------------------------------
TimeProcess::TimeProcess(wchar_t* names[], wchar_t* procName)
{
    const int sMaxCmd = 100;
    wchar_t perfCmd[sMaxCmd];

    for( ; *names != NULL; names++)
    {
        swprintf_s(perfCmd, sMaxCmd, *names, procName);
        PerfItem* pPerfItem = new PerfItem(perfCmd);
        m_pCounters.push_back(pPerfItem);
    }

    m_startTick = GetTickCount();
    PerfCounters::Collect();
}

//-----------------------------------------------------------------------------
TimeProcess::~TimeProcess()
{
    size_t cnt = m_pCounters.size();
    for (size_t idx = 0; idx < cnt; idx++)
        delete m_pCounters[idx];
}

//-----------------------------------------------------------------------------
bool TimeProcess::Collect()
{
    bool ok;
    if (ok = PerfCounters::Collect()) {
        size_t cnt = m_pCounters.size();
        for (size_t idx = 0; idx < cnt; idx++)
            m_pCounters[idx]->Collect();
    }
    return ok;
}

//-----------------------------------------------------------------------------
void TimeProcess::Display(FILE* out, wchar_t* fmt[])
{
    double elapsedSeconds = (GetTickCount() - m_startTick) / 1000.0;
    size_t cnt = m_pCounters.size();
    for(size_t idx = 0 ; fmt[idx] != NULL; idx++)
    {
        if (m_pCounters[idx]->m_collectionCount > 0) // only display if valid data collected.
        {
            if (m_pCounters[idx]->m_counter.IsPerSec())
                Printf2(out, fmt[idx], m_pCounters[idx]->m_total / m_pCounters[idx]->m_collectionCount * elapsedSeconds);
            else
                Printf2(out, fmt[idx], m_pCounters[idx]->m_total);
        }
    }
}
 
//-----------------------------------------------------------------------------
// Print to log and standard out.
void Printf2(FILE* log, wchar_t* wFmt, ...)
{
    va_list argp;
    va_start(argp, wFmt);
    if (log != NULL)
        vfwprintf(log, wFmt, argp);
    vfwprintf(stdout, wFmt, argp);
    va_end(argp);
}

//-----------------------------------------------------------------------------
void PrintFileTime2(FILE* log, wchar_t* pFmt, FILETIME ft)
{
    SYSTEMTIME st;
    const int szLen = 255;
    wchar_t szLocalDate[szLen], szLocalTime[szLen];
    szLocalDate[0] = _T('\0');
    szLocalTime[0] = _T('\0');

    if (ft.dwHighDateTime != 0 || ft.dwLowDateTime != 0)
    {
        FileTimeToLocalFileTime( &ft, &ft );
        FileTimeToSystemTime( &ft, &st );
        swprintf(szLocalDate, szLen, L"%4d/%02d/%02d",
            st.wYear, st.wMonth, st.wDay);
        swprintf(szLocalTime, szLen, L"%2d:%02d:%02d.%04d",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        // GetDateFormat( LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szLocalDate, 255 );
        // GetTimeFormat( LOCALE_USER_DEFAULT, 0, &st, NULL, szLocalTime, 255 );
    }

    if (log)
        fwprintf(log, pFmt, szLocalDate, szLocalTime );
    fwprintf(stdout, pFmt, szLocalDate, szLocalTime );
}

//-----------------------------------------------------------------------------
inline ULARGE_INTEGER FtToUl(FILETIME& ft)
{
    ULARGE_INTEGER ul;
    ul.HighPart = ft.dwHighDateTime;
    ul.LowPart = ft.dwLowDateTime;
    return ul;
}

//-----------------------------------------------------------------------------
ULARGE_INTEGER FILETIMEsub(FILETIME& left, FILETIME& right)
{
    ULARGE_INTEGER result;
    result.QuadPart = FtToUl(left).QuadPart - FtToUl(right).QuadPart;
    return result;
}