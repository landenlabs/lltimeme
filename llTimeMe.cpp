///
// llTimeMe v1.1  March 2010
//
// Author: Dennis Lang 2010
// https://lanenlabs.com
//
// llTimeMe program measures program execution statistics.
// Program is modeled after Microsoft's  Resource Kit 2003 TimeIt program which 
// fails to run on Windows7
//
// Sample output of my llTimeMe:
//
//
//    =llTimeMe
//    Thu Mar 04 20:17:59 2010 =Exit Time
//    systeminfo
//    
//    =Performance Counters:
//             22.3126  =Elapsed Seconds
//                   2  =%Process Usage
//                   1  =%User Time
//                1613  =Page Faults
//            40448000  =Virtual Byte Peak
//             5808128  =Working Set Peak
//              576863  =System Calls
//              141860  =System Context Switches
//             1971902  =System ~Bytes Read
//              425679  =System ~Bytes Written
//              863946  =System ~Bytes Other
//                   0  =Processor(s) % DPC Time
//                   0  =Processor(s) % Interrupt Time
//                  11  =Processor(s) % Privleged Time
//                  24  =Processor(s) % Processor Time
//                  13  =Processor(s) % User Time
//                3619  =Processor(s) ~DPCs Queued
//               25471  =Processor(s) ~Interrupts
//    
//    =Process Counters:
//       20:17:36.0689 =Creation
//       20:17:59.0002 =Exit
//        0:00:22.0312 =Elapsed
//                  49 =Handle Count
//                1624 =PageFaults
//             5808128 =Working Set Peak
//                   4 =Read IO calls
//                   2 =Write IO calls
//                 645 =Other IO calls
//
//
// Sample output of MS's TimeIt:
//
//     Version Number:   Windows NT 5.1 (Build 2600)
//     Exit Time:        10:54 am, Saturday, February 20 2010
//     Elapsed Time:     0:00:21.203
//     Process Time:     0:00:00.125
//     System Calls:       401361
//     Context Switches:    88011
//     Page Faults:         11811
//     Bytes Read:        1773428
//     Bytes Written:      231762
//     Bytes Other:        650072
//


#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>

#include <psapi.h>
#pragma comment( lib, "psapi.lib" )

#include "PerfCounters.h"
#include "TimeProcess.h"

extern void EnterDebugLoop(const LPDEBUG_EVENT DebugEv);
DEBUG_EVENT debugEv;


wchar_t* PerfCounterNames[] =
{
    L"\\Process(%s)\\Elapsed Time",
    L"\\Process(%s)\\%% Processor Time",
    L"\\Process(%s)\\%% User Time",
    L"\\Process(%s)\\Page Faults/sec",
    L"\\Process(%s)\\Virtual Bytes Peak",
    L"\\Process(%s)\\Working Set Peak",
    L"\\System\\System Calls/sec",
    L"\\System\\Context Switches/sec",
    L"\\System\\File Read Bytes/sec",
    L"\\System\\File Write Bytes/sec",
    L"\\System\\File Control Bytes/sec",
    L"\\Processor(_Total)\\%% DPC Time",
    L"\\Processor(_Total)\\%% Interrupt Time",
    L"\\Processor(_Total)\\%% Privileged Time",
    L"\\Processor(_Total)\\%% Processor Time",
    L"\\Processor(_Total)\\%% User Time",
    L"\\Processor(_Total)\\DPCs Queued/sec",
    L"\\Processor(_Total)\\Interrupts/sec",
    NULL
};

wchar_t* PerfFmts[] =
{
    L"%16.4f  =Elapsed Seconds\n",
    L"%16.0f  =%%Process Usage\n",
    L"%16.0f  =%%User Time\n",
    L"%16.0f  =Page Faults\n",
    L"%16.0f  =Virtual Byte Peak\n",
    L"%16.0f  =Working Set Peak\n",
    L"%16.0f  =System Calls\n",
    L"%16.0f  =System Context Switches\n",
    L"%16.0f  =System ~Bytes Read\n",
    L"%16.0f  =System ~Bytes Written\n",
    L"%16.0f  =System ~Bytes Other\n",
    L"%16.0f  =Processor(s) %% DPC Time\n",
    L"%16.0f  =Processor(s) %% Interrupt Time\n",
    L"%16.0f  =Processor(s) %% Privleged Time\n",
    L"%16.0f  =Processor(s) %% Processor Time\n",
    L"%16.0f  =Processor(s) %% User Time\n",
    L"%16.0f  =Processor(s) ~DPCs Queued\n",
    L"%16.0f  =Processor(s) ~Interrupts\n",
    NULL
};

//-----------------------------------------------------------------------------
static TCHAR* ProcName(TCHAR* pCmd, TCHAR* outProcName, int outSize)
{
    static TCHAR sAuxChar[] = _T("\"*:<>?\\/|");   // not valid in filename

    TCHAR* pDst = outProcName;
    TCHAR* pLastDot = _tcsrchr(pCmd, _T('.')); 
    TCHAR* pLastSlash = _tcsrchr(pCmd, _T('\\')); 

    if (pLastSlash != NULL)
        pCmd = ++pLastSlash;
    
    while (outSize-- != 0 && *pCmd >= _T(' '))
    {
        if (pCmd == pLastDot)
            break;
        *pDst++ = *pCmd++;
    }

    *pDst++  = _T('\0');
    return outProcName;
}

bool ToBool(BOOL b)
{
    return b == TRUE;
}

//-----------------------------------------------------------------------------
int _tmain( int argc, TCHAR *argv[] )
{
    if (argc == 1)
    {
        #define _VERSION "v1.4"
        char sUsage[] =
            "\n"
            "llTimeMe  " _VERSION " - " __DATE__ "\n"
            "By: Dennis Lang\n"
            "https://lanenlabs.com\n"
            "\n"
            "Measure Program Execution\n"
            "Usage: %S [programToTime] [programArgs]...\n";

        printf(sUsage, argv[0]);
        return -1;
    }

    // Initialize Performance Counters
    if (false == PerfCounters::Initialize())
    {
        fprintf(stderr, "Failed to initialize performance counters\n");
        return -2;
    }

    // Merge argv's into one command line, add appropriate quotes.
    TCHAR procName[4096];
    TCHAR cmdBuf[4096];
    TCHAR quote[] = _T("\"");
    TCHAR space = ' ';
    _tcscpy_s(cmdBuf, sizeof(cmdBuf)/sizeof(cmdBuf[0]), _T(""));
    for (int argIdx = 1; argIdx < argc; argIdx++)
    {
        if (argIdx != 1)
            _tcscat_s(cmdBuf, sizeof(cmdBuf)/sizeof(cmdBuf[0]), L" ");
        bool addQuote = (_tcschr(argv[argIdx], space) != NULL) && (argv[argIdx][0] != quote[0]);
        if (addQuote)
            _tcscat_s(cmdBuf, sizeof(cmdBuf)/sizeof(cmdBuf[0]), quote);
        _tcscat_s(cmdBuf, sizeof(cmdBuf)/sizeof(cmdBuf[0]), argv[argIdx]);
         if (addQuote)
            _tcscat_s(cmdBuf, sizeof(cmdBuf)/sizeof(cmdBuf[0]), quote);
    }

    // Start the child process.
    // Start suspended so counters can be attached and use debug to detected program exit.
    // DWORD crFlags = CREATE_SUSPENDED | DEBUG_ONLY_THIS_PROCESS;
    // DWORD crFlags = CREATE_SUSPENDED | DEBUG_PROCESS;
    DWORD crFlags = CREATE_SUSPENDED;
    
    IO_COUNTERS ioCounters;
    bool gotIoCounters = false;

    DWORD handleCount = 0;
    bool gotHndCounter = false;
    
    FILETIME creationTime, exitTime, kernelTime, userTime;
    bool gotProcTime = false;
    
    PROCESS_MEMORY_COUNTERS psMemCounters;
    bool gotMemInfo;

    STARTUPINFO si;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof(pi) );

    creationTime.dwHighDateTime = creationTime.dwLowDateTime = 0;
    kernelTime.dwHighDateTime = kernelTime.dwLowDateTime = 0;
    userTime.dwHighDateTime = userTime.dwLowDateTime = 0;
    exitTime.dwHighDateTime = exitTime.dwLowDateTime = 0;

    if( !CreateProcess( NULL,   // No module name (use command line)
        cmdBuf,         // Command line
        NULL,           // Process handle not inheritable    
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        crFlags,        // creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
            ) 
    {
        fwprintf(stderr, L"[Error %d] - Failed to start process %s\n", GetLastError(), cmdBuf );
        PerfCounters::Close();
        return GetLastError();
    }
    else {
        //fwprintf(stderr, L"Started %s\n", cmdBuf);
    }

    // Create counters and start program
    DWORD gotLen = GetProcessImageFileName(pi.hProcess, procName, sizeof(procName));
    if (gotLen == 0) {
        fwprintf(stderr, L"[Error %d] - Failed to start process %s\n", GetLastError(), cmdBuf);
        return GetLastError();
    }
    ProcName(procName, procName, ARRAYSIZE(procName));
    TimeProcess timeProcess(PerfCounterNames, procName);

    if (timeProcess.Collect()) {
        fprintf(stderr, "Counters before starting command\n");
        timeProcess.Display(stderr, PerfFmts);
    }

    ResumeThread(pi.hThread);

    if ((crFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)) != 0)
    {
        // Run program in debug mode to detect program exit
        // Note: debug mode may slow program's execution.
        debugEv.dwProcessId = pi.dwProcessId;
        debugEv.dwThreadId = pi.dwThreadId;
        EnterDebugLoop(&debugEv);

        // Program is pending to exit, quickly grab perf counters.
        timeProcess.Collect();

        gotIoCounters = ToBool(GetProcessIoCounters(pi.hProcess, &ioCounters));
        gotHndCounter = ToBool(GetProcessHandleCount(pi.hProcess, &handleCount));
        gotProcTime =  ToBool(GetProcessTimes(pi.hProcess, &creationTime, &exitTime, &kernelTime, &userTime));
        gotMemInfo = ToBool(GetProcessMemoryInfo(pi.hProcess, &psMemCounters, sizeof(psMemCounters)));
    }
    else
    {
        // Wait until child process exits.
        while (WaitForSingleObject( pi.hProcess, 10) == WAIT_TIMEOUT)
        {
            // Last call to collect will fail since process exited.
            timeProcess.Collect();
        }

        gotIoCounters = ToBool(GetProcessIoCounters(pi.hProcess, &ioCounters));
        gotHndCounter = ToBool(GetProcessHandleCount(pi.hProcess, &handleCount));
        gotProcTime =  ToBool(GetProcessTimes(pi.hProcess, &creationTime, &exitTime, &kernelTime, &userTime));
        gotMemInfo = ToBool(GetProcessMemoryInfo(pi.hProcess, &psMemCounters, sizeof(psMemCounters)));
    }

    // Close process and thread handles. 
    PerfCounters::Close();
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Print results and save in log file.
    time_t nowTime;
    time(&nowTime);
    struct tm * timeinfo = localtime(&nowTime);
    FILE* log = fopen("llTimeMe.log", "a");   

    Printf2(log, L"\n=llTimeMe\n");
    Printf2(log, L"%.24S =Exit Time\n", asctime(timeinfo));
    Printf2(log, L"%s\n", cmdBuf);

    Printf2(log, L"\n=Performance Counters:\n");
    timeProcess.Display(log, PerfFmts);
    Printf2(log, L"\n=Process Counters:\n");
    
    if (gotProcTime && userTime.dwLowDateTime != 0)
    {
        PrintFileTime2(log, L"%.0s%16s =Creation\n", creationTime);
        // PrintFileTime2(log, L"    Kernel:   %s\n", kernelTime);
        // PrintFileTime2(log, L"    User:     %s\n", userTime);
        PrintFileTime2(log, L"%.0s%16s =Exit\n", exitTime);
      
        if (CompareFileTime(&creationTime, &exitTime) < 0)
        {
            ULARGE_INTEGER tenMicroseconds = FILETIMEsub(exitTime, creationTime);
            double seconds = tenMicroseconds.QuadPart / 1e7;
            Printf2(log, L"   %2d:%02d:%02d.%04d =Elapsed\n", 
                int(seconds/3600)%24, 
                int(seconds/60)%60,
                int(seconds)%60 ,
                int((seconds - int(seconds))*1000));
        }
    }  
    
    if (gotHndCounter)
        Printf2(log, L"%16d =Handle Count\n", handleCount);
    
    if (gotMemInfo)
    {
        Printf2(log, L"%16d =PageFaults\n", psMemCounters.PageFaultCount);
        Printf2(log, L"%16d =Working Set Peak\n", psMemCounters.PeakWorkingSetSize);
    }
    
    if (gotIoCounters)
    {
        Printf2(log, L"%16u =Read IO calls\n", ioCounters.ReadOperationCount);
        Printf2(log, L"%16u =Write IO calls\n", ioCounters.WriteOperationCount);
        Printf2(log, L"%16u =Other IO calls\n", ioCounters.OtherOperationCount);
    }

    return 0;
}

#if 0
//-----------------------------------------------------------------------------
static void GetProcessName(DWORD processId)
{
    DWORD accAttr = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
    HANDLE hprocess2 = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, processId);

    wchar_t procName[256];
    memset(procName, 0, sizeof(procName));
    DWORD st;

    //st = GetProcessImageFileName(hprocess2, procName, 256);
    HMODULE hMod;
    DWORD cbNeeded = 0;

    if (EnumProcessModules(hprocess2, &hMod, sizeof(hMod), &cbNeeded))
    {
        st = GetModuleBaseName(hprocess2, hMod, procName, sizeof(procName)/sizeof(procName[0]));
    }

    // st = GetModuleFileNameEx(hprocess2, 0, procName, 256);
    // st = GetModuleFileName(pi.hProcess, &procName, sizeof(procName));
    // st = GetModuleBaseName(hprocess2, NULL, procName, sizeof(procName)/sizeof(procName[0]));
    DWORD err = GetLastError();
    CloseHandle(hprocess2);
}
#endif