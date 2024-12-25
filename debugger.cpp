///
// The following routine is used just to keep the process under debug mode running
// until it exits. 
//

///
// Sample Debug code from MS website.
// http://msdn.microsoft.com/en-us/library/ms681675(VS.85).aspx
//

#include <windows.h>
#include <iostream>
#include <sstream>
#include <string>


DWORD OnCreateThreadDebugEvent(const LPDEBUG_EVENT) { return DBG_CONTINUE; }
DWORD OnCreateProcessDebugEvent(const LPDEBUG_EVENT){ return DBG_CONTINUE; }
DWORD OnExitThreadDebugEvent(const LPDEBUG_EVENT){ return DBG_CONTINUE; }
DWORD OnExitProcessDebugEvent(const LPDEBUG_EVENT){ return DBG_CONTINUE; }
DWORD OnLoadDllDebugEvent(const LPDEBUG_EVENT){ return DBG_CONTINUE; }
DWORD OnUnloadDllDebugEvent(const LPDEBUG_EVENT){ return DBG_CONTINUE; }
DWORD OnOutputDebugStringEvent(const LPDEBUG_EVENT){ return DBG_CONTINUE; }
DWORD OnRipEvent(const LPDEBUG_EVENT){ return DBG_CONTINUE; }

void EnterDebugLoop(const LPDEBUG_EVENT DebugEv)
{
    DWORD dwContinueStatus = DBG_CONTINUE; // exception continuation 

    for(;;) 
    { 
        // Wait for a debugging event to occur. The second parameter indicates
        // that the function does not return until a debugging event occurs. 

        WaitForDebugEvent(DebugEv, INFINITE);

#ifdef DEBUG
        std::wostringstream wout;
        wout << "Event:"  << DebugEv->dwDebugEventCode << "\n";
        OutputDebugString(wout.str().c_str());
#endif

        // Process the debugging event code.
        switch (DebugEv->dwDebugEventCode) 
        { 
        case EXCEPTION_DEBUG_EVENT: 
            // Process the exception code. When handling 
            // exceptions, remember to set the continuation 
            // status parameter (dwContinueStatus). This value 
            // is used by the ContinueDebugEvent function. 

            switch(DebugEv->u.Exception.ExceptionRecord.ExceptionCode)
            { 
            case EXCEPTION_ACCESS_VIOLATION: 
                // First chance: Pass this on to the system. 
                // Last chance: Display an appropriate error. 
                break;

            case EXCEPTION_BREAKPOINT: 
                // First chance: Display the current 
                // instruction and register values. 
                break;

            case EXCEPTION_DATATYPE_MISALIGNMENT: 
                // First chance: Pass this on to the system. 
                // Last chance: Display an appropriate error. 
                break;

            case EXCEPTION_SINGLE_STEP: 
                // First chance: Update the display of the 
                // current instruction and register values. 
                break;

            case DBG_CONTROL_C: 
                // First chance: Pass this on to the system. 
                // Last chance: Display an appropriate error. 
                break;

            default:
                // Handle other exceptions. 
                break;
            } 

            break;

        case CREATE_THREAD_DEBUG_EVENT: 
            // As needed, examine or change the thread's registers 
            // with the GetThreadContext and SetThreadContext functions; 
            // and suspend and resume thread execution with the 
            // SuspendThread and ResumeThread functions. 

            dwContinueStatus = OnCreateThreadDebugEvent(DebugEv);
            break;

        case CREATE_PROCESS_DEBUG_EVENT: 
            // As needed, examine or change the registers of the
            // process's initial thread with the GetThreadContext and
            // SetThreadContext functions; read from and write to the
            // process's virtual memory with the ReadProcessMemory and
            // WriteProcessMemory functions; and suspend and resume
            // thread execution with the SuspendThread and ResumeThread
            // functions. Be sure to close the handle to the process image
            // file with CloseHandle.

            dwContinueStatus = OnCreateProcessDebugEvent(DebugEv);
            break;

        case EXIT_THREAD_DEBUG_EVENT: 
            // Display the thread's exit code. 

            dwContinueStatus = OnExitThreadDebugEvent(DebugEv);
            break;

        case EXIT_PROCESS_DEBUG_EVENT: 
            // Display the process's exit code. 

            dwContinueStatus = OnExitProcessDebugEvent(DebugEv);
            return;     // #### RETURN ####
            break;

        case LOAD_DLL_DEBUG_EVENT: 
            // Read the debugging information included in the newly 
            // loaded DLL. Be sure to close the handle to the loaded DLL 
            // with CloseHandle.

            dwContinueStatus = OnLoadDllDebugEvent(DebugEv);
            break;

        case UNLOAD_DLL_DEBUG_EVENT: 
            // Display a message that the DLL has been unloaded. 

            dwContinueStatus = OnUnloadDllDebugEvent(DebugEv);
            break;

        case OUTPUT_DEBUG_STRING_EVENT: 
            // Display the output debugging string. 

            dwContinueStatus = OnOutputDebugStringEvent(DebugEv);
            break;

        case RIP_EVENT:
            dwContinueStatus = OnRipEvent(DebugEv);
            break;
        } 

        // Resume executing the thread that reported the debugging event. 

        ContinueDebugEvent(DebugEv->dwProcessId, 
            DebugEv->dwThreadId, 
            dwContinueStatus);
    }
}