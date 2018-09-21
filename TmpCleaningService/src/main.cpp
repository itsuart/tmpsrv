#include <Windows.h>
#include "TmpCleaningService.hpp"

static const wchar_t* SVCNAME = L"TmpCleaningService";

//INSTALL/UNINSTALL & UI CODE

static bool ask(const wchar_t* question) {
    return IDYES == MessageBoxW(nullptr, question, SVCNAME, MB_YESNO | MB_ICONQUESTION);
}

static void inform(const wchar_t* message) {
    MessageBoxW(nullptr, message, SVCNAME, MB_OK | MB_ICONINFORMATION);
}

static void error(const wchar_t* message) {
    MessageBoxW(nullptr, message, SVCNAME, MB_OK | MB_ICONERROR);
}

static void warn(const wchar_t* message) {
    MessageBoxW(nullptr, message, SVCNAME, MB_OK | MB_ICONWARNING);
}

static void InstallService(wchar_t* processFullPath) {
    SC_HANDLE schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (NULL == schSCManager) {
        error(L"Failed to open Service Control manager.");
        return;
    }

    wchar_t szPath[MAX_PATH + 100];
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

    if (schService == NULL) {
        error(L"CreateService failed.");
        CloseServiceHandle(schSCManager);
        return;
    }
    else {
        // Set the description
        SERVICE_DESCRIPTIONW sd;

        sd.lpDescription = const_cast<wchar_t*>(L"Service that cleans c:\\tmp folder on system shutdown or reboot");

        if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd)) {
            warn(L"Failed to set proper description for the service.\n");
        }

        if (ask(L"Service installed successfully. Would you like to start it right now?")) {
            if (!StartServiceW(schService, 0, NULL)) {
                error(L"Failed to start the service\n");
            }
            else {
                inform(L"Service started\n");
            }
        }
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

static void UninstallService() {
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (NULL == schSCManager){
        error(L"OpenSCManager failed. Please, check, that you are running this under Administrator account.\n");
        return;
    }

    SC_HANDLE schService = OpenService(schSCManager, SVCNAME, SERVICE_STOP | DELETE);

    if (schService == NULL){
        error(L"OpenService failed. Please, check, that you are running this under Administrator account.\n");
        CloseServiceHandle(schSCManager);
        return;
    }

    // Stop the service
    SERVICE_STATUS dummy;
    ControlService(schService, SERVICE_CONTROL_STOP, &dummy);

    // Delete the service.
    if (!DeleteService(schService)) {
        error(L"DeleteService failed\n");
    }
    else {
        inform(L"Service stopped and deleted successfully\n");
    }
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
}

static void RunService() {
    SERVICE_TABLE_ENTRY entries[] = {
        TmpCleaningService::GetServiceTableEntry(),
        {nullptr, nullptr}
    };

    if (!StartServiceCtrlDispatcherW(entries)) { //will block untill all services are stopped
        error(L"StartServiceCtrlDispatcher call failed");
        return;
    }
}

static void run() {
    wchar_t processFullPath[MAX_PATH + 2] = {0};

    DWORD processFullPathLength = GetModuleFileNameW(NULL, &processFullPath[0], MAX_PATH + 1);
    if (MAX_PATH + 1 == processFullPathLength) {
        error(L"Failed to determine process full path.");
        return;
    }

    wchar_t* commandLine = GetCommandLineW();
    LPWSTR* szArglist = NULL;
    int nArgs = 0;

    szArglist = CommandLineToArgvW(commandLine, &nArgs);
    if (NULL == szArglist)
    {
        error(L"CommandLineToArgvW failed");
        return;
    }

    if (nArgs == 1) {
        SC_HANDLE hServiceController = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
        if (hServiceController == NULL) {
            error(L"Failed to open ServiceControlManager. Please run me as administrator.");
            //goto exit;
            return;
        }

        const SC_HANDLE hService = OpenServiceW(hServiceController, SVCNAME, SC_MANAGER_CONNECT);
        if (hService != NULL) {
            //we have service installed
            if (ask(L"Would you like to uninstall the service?")) {
                CloseServiceHandle(hService);
                CloseServiceHandle(hServiceController);

                ShellExecuteW(NULL, L"runas", processFullPath, L"-u", NULL, SW_SHOWDEFAULT);
                return;
            }
        }
        else {
            const DWORD lastError = GetLastError();
            if (lastError == ERROR_SERVICE_DOES_NOT_EXIST) {
                if (ask(L"Would you like to install the service?")) {
                    CloseServiceHandle(hServiceController);

                    ShellExecuteW(NULL, L"runas", processFullPath, L"-i", NULL, SW_SHOWDEFAULT);
                    return;
                }
            }
            else {
                error(L"Failed to query Service Database");
                return;
            }
        }
        return;
    }

    if (nArgs == 2) {
        wchar_t* arg = szArglist[1];
        if (lstrcmpiW(arg, L"-w") == 0) {
            RunService();
        }
        else if (lstrcmpiW(arg, L"-i") == 0) {
            InstallService(processFullPath);
        }
        else if (lstrcmpiW(arg, L"-u") == 0) {
            UninstallService();
        }
        else if (lstrcmpiW(arg, L"-a") == 0) {
            //auto usercount = TmpCleaningService::CountLoggedInUsers();
        }
        else {
            error(L"Invalid argument. Use '-i' to install the service and '-u' to uninstall it.");
        }
        return;
    }

    error(L"Too many arguments!");

    LocalFree(szArglist);
}

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
) {
    run();
    return 0;
}
