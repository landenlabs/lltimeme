///
// The following is not an active part of llTimeMe program.
// The following module can be used to get a performance counter name.
//


#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <tchar.h>

#pragma comment(lib, "pdh.lib")

CONST ULONG SAMPLE_INTERVAL_MS    = 1000;
CONST PWSTR BROWSE_DIALOG_CAPTION = L"Select a counter to monitor.";

//-----------------------------------------------------------------------------
void PromptPerfCounters()
{
    PDH_STATUS Status;
    HQUERY Query = NULL;
    PDH_BROWSE_DLG_CONFIG BrowseDlgData;
    WCHAR CounterPathBuffer[PDH_MAX_COUNTER_PATH];

    //
    // Create a query.
    //

    Status = PdhOpenQuery(NULL, NULL, &Query);
    if (Status != ERROR_SUCCESS) 
    {
       wprintf(L"\nPdhOpenQuery failed with status 0x%x.", Status);
       goto Cleanup;
    }

    //
    // Initialize the browser dialog window settings.
    //

    ZeroMemory(&CounterPathBuffer, sizeof(CounterPathBuffer));

    while (1)
    {
        ZeroMemory(&BrowseDlgData, sizeof(PDH_BROWSE_DLG_CONFIG));

        BrowseDlgData.bIncludeInstanceIndex = FALSE;
        BrowseDlgData.bSingleCounterPerAdd = TRUE;
        BrowseDlgData.bSingleCounterPerDialog = TRUE;
        BrowseDlgData.bLocalCountersOnly = FALSE;
        BrowseDlgData.bWildCardInstances = TRUE;
        BrowseDlgData.bHideDetailBox = TRUE;
        BrowseDlgData.bInitializePath = FALSE;
        BrowseDlgData.bDisableMachineSelection = FALSE;
        BrowseDlgData.bIncludeCostlyObjects = FALSE;
        BrowseDlgData.bShowObjectBrowser = FALSE;
        BrowseDlgData.hWndOwner = NULL;
        BrowseDlgData.szReturnPathBuffer = CounterPathBuffer;
        BrowseDlgData.cchReturnPathLength = PDH_MAX_COUNTER_PATH;
        BrowseDlgData.pCallBack = NULL;
        BrowseDlgData.dwCallBackArg = 0;
        BrowseDlgData.CallBackStatus = ERROR_SUCCESS;
        BrowseDlgData.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
        BrowseDlgData.szDialogBoxCaption = BROWSE_DIALOG_CAPTION;

        //
        // Display the counter browser window. The dialog is configured
        // to return a single selection from the counter list.
        //

        Status = PdhBrowseCounters(&BrowseDlgData);

        if (Status != ERROR_SUCCESS) 
        {
            if (Status == PDH_DIALOG_CANCELLED) 
            {
                wprintf(L"\nDialog canceled by user.");
            }
            else 
            {
                wprintf(L"\nPdhBrowseCounters failed with status 0x%x.", Status);
            }
            goto Cleanup;
        } 
        else if (wcslen(CounterPathBuffer) == 0) 
        {
            wprintf(L"\nUser did not select any counter.");
            goto Cleanup;
        }
        else
        {
            wprintf(L"\nCounter selected: %s\n", CounterPathBuffer);
        }

        //
        // Add the selected counter to the query.
        //
        OutputDebugString(CounterPathBuffer);
        OutputDebugString(L"\n");
        wprintf(L"%s\n", CounterPathBuffer);
    }

Cleanup:

    //
    // Close the query.
    //

    if (Query) 
    {
       PdhCloseQuery(Query);
    }
}