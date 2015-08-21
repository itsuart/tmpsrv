#define UNICODE

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <stdbool.h>

static USHORT* TempDir = L"c:\\tmp";
static USHORT* TempDirContent = L"c:\\tmp\\*\0";
static USHORT* SVCNAME = L"tmpsrv";

#define ServiceDescripton L"Deletes content of c:\\tmp directory upon system shutdown and service start."

/* UTILITIES */
static void CreateTmpFolder(){
    DeleteFileW(TempDir);
    CreateDirectoryW(TempDir, NULL);
}

static void DeleteTmpFolder(){
    SHFILEOPSTRUCT op = {0};
    op.hwnd = NULL;
    op.wFunc = FO_DELETE;
    op.pFrom = TempDirContent;
    op.pTo = NULL;
    op.fFlags =  FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
    SHFileOperationW (&op);

    DeleteFileW(TempDir);
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

static bool ask(USHORT* question){
    return IDYES == MessageBoxW(NULL, question, SVCNAME, MB_YESNO | MB_ICONQUESTION);
}

static void inform (USHORT* message){
    MessageBoxW(NULL, message, SVCNAME, MB_OK | MB_ICONINFORMATION);
}

static void error(USHORT* message){
    MessageBoxW(NULL, message, SVCNAME, MB_OK | MB_ICONERROR);
}

static void warn(USHORT* message){
    MessageBoxW(NULL, message, SVCNAME, MB_OK | MB_ICONWARNING);
}

static void install_service_and_start(USHORT* processFullPath){
    SC_HANDLE schSCManager = OpenSCM();
    if (NULL == schSCManager){
        error(L"OpenSCManager failed\n");
        return;
    }

    USHORT szPath[MAX_PATH + 100];
    SecureZeroMemory(szPath, sizeof(szPath));
    
    wsprintfW(szPath, L"\"%s\" -w", processFullPath);

    SC_HANDLE schService = CreateServiceW( 
        schSCManager,              // SCM database 
        SVCNAME,                   // name of service 
        SVCNAME,                   // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_AUTO_START,      // Autostart with OS
        SERVICE_ERROR_IGNORE,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 
 
    if (schService == NULL){
        error(L"CreateService failed."); 
        CloseServiceHandle(schSCManager);
        return;
    }
    else {
        // Set the description
        SERVICE_DESCRIPTION sd;

        sd.lpDescription = ServiceDescripton;

        if( !ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd)){
            warn(L"Failed to set proper description for the service.\n");
        }

        if (ask(L"Service installed successfully. Would you like to start it right now?")){
            if (!StartServiceW(schService, 0, NULL)){
                error(L"Failed to start the service\n");
            }else{
                inform(L"Service started\n");
            }
        } 
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

static void stop_service_and_delete(){
    SC_HANDLE schSCManager = OpenSCM();

    if (NULL == schSCManager) 
    {
        error(L"OpenSCManager failed. Please, check, that you are running this under Administrator account.\n");
        return;
    }

    SC_HANDLE schService = OpenService (schSCManager, SVCNAME, SERVICE_STOP | DELETE);
 
    if (schService == NULL)
    { 
        error(L"OpenService failed. Please, check, that you are running this under Administrator account.\n"); 
        CloseServiceHandle(schSCManager);
        return;
    }

    // Stop the service
    SERVICE_STATUS dummy;
    ControlService (schService, SERVICE_CONTROL_STOP, &dummy);

    // Delete the service.
    if (!DeleteService(schService)){
        error(L"DeleteService failed\n"); 
    }else{
        inform(L"Service stopped and deleted successfully\n"); 
    }
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

void entry_point(){ //ENTRY POINT
    USHORT processFullPath [MAX_PATH + 2];
    SecureZeroMemory(processFullPath, sizeof(processFullPath));
    DWORD processFullPathLength = GetModuleFileNameW(NULL, (USHORT*)&processFullPath, MAX_PATH + 1);
    if (MAX_PATH + 1 == processFullPathLength){
        error(L"Failed to determine process full path.");
        return;
    }
    
    USHORT* commandLine = GetCommandLineW();
    LPWSTR* szArglist = NULL;
    int nArgs = 0;

    szArglist = CommandLineToArgvW(commandLine, &nArgs);
    if(NULL == szArglist )
    {
        error(L"CommandLineToArgvW failed");
        return;
    }

    if (nArgs == 1){
        SC_HANDLE hServiceController = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
        if (hServiceController == NULL){
            error(L"Failed to open ServiceControlManager. Please run me as administrator.");
            //goto exit;
            return;
        }

        SC_HANDLE hService = OpenServiceW(hServiceController, SVCNAME, SC_MANAGER_CONNECT);
        if (hService != NULL){
            //we have service installed
            if (ask(L"Would you like to uninstall tmpsrv service?")){
                CloseServiceHandle(hService);
                CloseServiceHandle(hServiceController);

                ShellExecuteW(NULL, L"runas", processFullPath, L"-u", NULL, SW_SHOWDEFAULT);
                return;
            }
        } else {
            DWORD lastError = GetLastError();
            if (lastError == ERROR_SERVICE_DOES_NOT_EXIST){
                if (ask(L"Would you like to install tmpsrv service?")){
                    CloseServiceHandle(hServiceController);

                    ShellExecuteW(NULL, L"runas", processFullPath, L"-i", NULL, SW_SHOWDEFAULT);
                    return;
                }
            } else {
                error(L"Failed to query Service Database");
                return;
            }
        }
        return;
    }

    if (nArgs == 2){
        USHORT* arg = szArglist[1];
        if (lstrcmpiW(arg, L"-w") == 0){
            SvcRun();
        } else if (lstrcmpiW (arg, L"-i") == 0){
            install_service_and_start(processFullPath);
        } else if (lstrcmpiW (arg, L"-u") == 0){
            stop_service_and_delete();
        } else {
            error(L"Invalid argument. Use '-i' to install the service and '-u' to uninstall it.");
        }
        return;
    }

    error(L"Too many arguments!");
    
    LocalFree(szArglist);
}
