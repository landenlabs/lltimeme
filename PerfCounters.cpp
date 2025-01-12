///
// Author: Dennis Lang 2010
// https://lanenlabs.com
//
                
///  Class to Collect Performance counter stats.
//   Usage:
//      1. Call Initialize() once program run.
//      2. Create one or more PerfCounters(counterName)
//      3. Call Collect() 
//      4. Call GetValue() as often as you want.
//      5. Call Close() once per program run.

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <tchar.h>

#pragma comment(lib, "pdh.lib")

#include "PerfCounters.h"

HQUERY PerfCounters::m_sQuery;
const double PerfCounters::m_sNoDouble = 0;
 

//-----------------------------------------------------------------------------
PerfCounters::PerfCounters(TCHAR* counterName)  
{
    _tcscpy_s(m_cmd, sizeof(m_cmd)/sizeof(m_cmd[0]), counterName);
    m_status = PdhAddCounter(m_sQuery, counterName, 0, &m_counter);
    if (m_status != ERROR_SUCCESS) 
    {
        _ftprintf(stderr, _T("\nPdhAddCounter %s failed with status 0x%x."), m_cmd, m_status);
    }

    m_isPerSec = (_tcsstr(counterName, _T("/sec")) != NULL);
}

//-----------------------------------------------------------------------------
bool PerfCounters::GetValue(double& doubleValue)
{
    if (m_status == ERROR_SUCCESS) 
    {
        PDH_FMT_COUNTERVALUE DisplayValue;
        DWORD CounterType;
        m_status = PdhGetFormattedCounterValue(m_counter, PDH_FMT_DOUBLE, &CounterType, &DisplayValue);
        doubleValue = (m_status == ERROR_SUCCESS) ? DisplayValue.doubleValue :  m_sNoDouble;
    }

    return (m_status == ERROR_SUCCESS);
}

//-----------------------------------------------------------------------------
void PerfCounters::DisplayErrorStatus(FILE* log)
{
    if (m_status != ERROR_SUCCESS) 
    {
        switch (m_status)
        {
        case PDH_INVALID_ARGUMENT:
            _ftprintf(log, _T("\nCounterValue %s failed, invalid Argument\n"), m_cmd);
            break;
        case PDH_INVALID_DATA:
            _ftprintf(log, _T("\nCounterValue %s failed, invalid data\n"), m_cmd);
            break;
        case PDH_INVALID_HANDLE:
            _ftprintf(log, _T("\nCounterValue %s failed, invalid handle\n"), m_cmd);
            break;
        default:
            _ftprintf(log, _T("\nCounterValue %s failed with status 0x%x."), m_cmd, m_status);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
bool PerfCounters::Initialize()
{
    PDH_STATUS Status = PdhOpenQuery(NULL, NULL, &m_sQuery);
    if (Status != ERROR_SUCCESS) 
    {
       _ftprintf(stderr, _T("\nPdhOpenQuery failed with status 0x%x."), Status);
       return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
bool PerfCounters::Collect()
{
    //
    // Most counters require two sample values to display a formatted value.
    // PDH stores the current sample value and the previously collected
    // sample value. This call retrieves the first value that will be used
    // by PdhGetFormattedCounterValue in the first iteration of the loop
    // Note that this value is lost if the counter does not require two
    // values to compute a displayable value.
    //

    PDH_STATUS Status = PdhCollectQueryData(m_sQuery);
    if (Status != ERROR_SUCCESS) 
    {
        fprintf(stderr, "\nPdhCollectQueryData failed with 0x%x.\n", Status);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
void PerfCounters::Close()
{
    if (m_sQuery) 
    {
       PdhCloseQuery(m_sQuery);
    }
}
