//Good article about Windows Services: http://www.devx.com/cplus/Article/9857
#define UNICODE

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>

static USHORT* TempDir = L"c:\\tmp";
static USHORT* TempDirContent = L"c:\\tmp\\*\0";

#define SVCNAME L"tmpsrv"
#define ServiceDescripton L"Deletes content of c:\\tmp directory upon system shutdown and service start."

/* UTILITIES */

static void WriteString(USHORT* string){
    DWORD dummy;
    HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteConsoleW(stdout, string, lstrlenW(string), &dummy, NULL);
}

static void CreateTmpFolder(){
    DeleteFileW(TempDir);
    CreateDirectoryW(TempDir, NULL);
}

static void DeleteTmpFolder(){
    DeleteFileW(TempDirContent);

    SHFILEOPSTRUCT op = {0};
    op.hwnd = NULL;
    op.wFunc = FO_DELETE;
    op.pFrom = TempDirContent;
    op.pTo = NULL;
    op.fFlags =  FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
    SHFileOperationW (&op);
}

static void CleanTmp(){
    //clean /tmp
    DeleteTmpFolder();
    CreateTmpFolder();
}

/* SERVICE STUFF */

SERVICE_STATUS          gSvcStatus; 
SERVICE_STATUS_HANDLE   gSvcStatusHandle; 
HANDLE                  ghSvcStopEvent = NULL;

static void ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint){
    static DWORD dwCheckPoint = 1;

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING){
        gSvcStatus.dwControlsAccepted = 0;
    } else {
        gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    if ( (dwCurrentState == SERVICE_RUNNING) ||
         (dwCurrentState == SERVICE_STOPPED) ){
        gSvcStatus.dwCheckPoint = 0;
    } else {
        gSvcStatus.dwCheckPoint = dwCheckPoint++;
    }

    // Report the status of the service to the SCM.
    SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}

static void DoWork(){
    CleanTmp();

    //wait for stop signal from SCM
    WaitForSingleObject(ghSvcStopEvent, INFINITE);

    ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
}

static void WINAPI SvcCtrlHandler (DWORD dwCtrl){
    // Handle the requested control code. 

    switch(dwCtrl){  
    case SERVICE_CONTROL_STOP: 
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

        // Signal the service to stop.
        SetEvent(ghSvcStopEvent);
        ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
        return;
    
    case SERVICE_CONTROL_SHUTDOWN:
        SetEvent(ghSvcStopEvent);
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        
        CleanTmp();
        return;
        
    default: 
        break;
    } 
}

// Called By ServiceDispatcher of SCM
static void WINAPI SvcMain (DWORD dwArgc, LPTSTR *lpszArgv){
    // Tell Service Dispatcher, that SvcCtrlHandler will handle request from SCM

    gSvcStatusHandle = RegisterServiceCtrlHandler (SVCNAME, SvcCtrlHandler);

    if (!gSvcStatusHandle){ 
        WriteString(L"Failed to register SCM Control Handler\nMaybe you running me not under Administrator account?\n");
        return; 
    } 

    // These SERVICE_STATUS members remain as set here

    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    gSvcStatus.dwServiceSpecificExitCode = 0;    

    // Report initial status to the SCM

    ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );

    // Create event to wait on
    // The control handler function, SvcCtrlHandler,
    // signals this event when it receives the stop control code.

    ghSvcStopEvent = CreateEvent(
        NULL,    // default security attributes
        TRUE,    // manual reset event
        FALSE,   // not signaled
        NULL);   // no name

    if (ghSvcStopEvent == NULL){
        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return; 
    }

    // Report running status when initialization is complete.
    ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

    DoWork();
}

static SC_HANDLE OpenSCM(){
    return OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
}

static void __stdcall SvcStopAndDelete(){
    SC_HANDLE schSCManager = OpenSCM();

    if (NULL == schSCManager) 
    {
        WriteString(L"OpenSCManager failed. Please, check, that you are running this under Administrator account.\n");
        return;
    }

    SC_HANDLE schService = OpenService (schSCManager, SVCNAME, SERVICE_STOP | DELETE);
 
    if (schService == NULL)
    { 
        WriteString(L"OpenService failed. Please, check, that you are running this under Administrator account.\n"); 
        CloseServiceHandle(schSCManager);
        return;
    }

    // Stop the service
    SERVICE_STATUS dummy;
    if (!ControlService (schService, SERVICE_CONTROL_STOP, &dummy)){
        WriteString(L"Failed to stop the service.\n");
    } else {
        WriteString(L"Service stopped.\n");
    }

    // Delete the service.
    if (!DeleteService(schService)){
        WriteString(L"DeleteService failed\n"); 
    }else{
        WriteString(L"Service deleted successfully\n"); 
    }
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

//   Installs a service in the SCM database
static void SvcInstallAndStart(){
    TCHAR szPath[MAX_PATH + 1];
    if (!GetModuleFileNameW (NULL, szPath, MAX_PATH)){
        WriteString(L"Failed to get name of the executable\n");
        return;
    }

    SC_HANDLE schSCManager = OpenSCM();
    if (NULL == schSCManager){
        WriteString(L"OpenSCManager failed\n");
        return;
    }

    SC_HANDLE schService = CreateService( 
        schSCManager,              // SCM database 
        SVCNAME,                   // name of service 
        SVCNAME,                   // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_AUTO_START,      // Autostart with OS
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 
 
    if (schService == NULL){
        WriteString(L"CreateService failed.\n"); 
        CloseServiceHandle(schSCManager);
        return;
    }
    else {
        // Set the description
        SERVICE_DESCRIPTION sd;

        sd.lpDescription = ServiceDescripton;

        if( !ChangeServiceConfig2(
                schService,                 // handle to service
                SERVICE_CONFIG_DESCRIPTION, // change: description
                &sd) )                      // new description
        {
            WriteString(L"Failed to set proper description for the service.\n");
        }

        WriteString(L"Service installed successfully\n"); 
     
        // Start the service

        if (!StartServiceW(schService, 0, NULL)){
            WriteString(L"Failed to start the service\n");
        }else{
            WriteString(L"Service started\n");
        }
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

static void SvcRun(){
    SERVICE_TABLE_ENTRY DispatchTable[] = 
        { 
            { SVCNAME, (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
            { NULL, NULL } //End of table mark
        }; 
 
    // This call returns when the service has stopped. 
    // The process should simply terminate when the call returns.
    StartServiceCtrlDispatcher(DispatchTable);
}

static void RealMain(USHORT* mode){
    if (NULL == mode){
        SvcRun();
    } else if (lstrcmpiW (mode, L"install") == 0){
        SvcInstallAndStart();
    } else if (lstrcmpiW (mode, L"uninstall") == 0){
        SvcStopAndDelete();
    } else {
        WriteString(L"Unknown mode. Use 'install' to install the service and 'uninstall' to uninstall it.");
    }
}

void Main(){ //ENTRY POINT
    USHORT* commandLine = GetCommandLineW();
    LPWSTR *szArglist = NULL;
    int nArgs = 0;

    szArglist = CommandLineToArgvW(commandLine, &nArgs);
    if( NULL == szArglist )
    {
        WriteString(L"CommandLineToArgvW failed\n");
        goto exit;
    }
    
    RealMain(nArgs < 2 ? NULL : szArglist[1]);
    LocalFree(szArglist);

exit:
    ExitProcess(0);
}
